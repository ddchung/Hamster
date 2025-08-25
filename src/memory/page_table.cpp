// Hamster page table implementation

#include <memory/page_manager.hpp>
#include <memory/page_table.hpp>
#include <memory/allocator.hpp>
#include <utility>
#include <cassert>
#include <cstring>
#include <array>

namespace Hamster
{   
    namespace
    {
        constexpr size_t LEAF_TOTAL_BITS = HAMSTER_PAGETABLE_LEAF_BITS + HAMSTER_PAGE_SIZE_BITS;
        constexpr size_t L2_TOTAL_BITS = HAMSTER_PAGETABLE_L2_BITS + LEAF_TOTAL_BITS;
        constexpr size_t L1_TOTAL_BITS = HAMSTER_PAGETABLE_L1_BITS + L2_TOTAL_BITS;

        constexpr auto LEAF_EMPTY_ARRAY = [] {
            std::array<uint32_t, 1 << HAMSTER_PAGETABLE_LEAF_BITS> arr;
            arr.fill(PageTable::PAGE_ID_UNUSED);
            return arr;
        }();
        constexpr void *L2_EMPTY_ARRAY[1 << HAMSTER_PAGETABLE_L2_BITS] = {};

        size_t l1_index(uint32_t address)
        {
            // No need to mask because L1 is at the top
            return address >> L2_TOTAL_BITS;
        }

        size_t l2_index(uint32_t address)
        {
            return (address >> LEAF_TOTAL_BITS) & ((1 << HAMSTER_PAGETABLE_L2_BITS) - 1);
        }

        size_t leaf_index(uint32_t address)
        {
            return (address >> HAMSTER_PAGE_SIZE_BITS) & ((1 << HAMSTER_PAGETABLE_LEAF_BITS) - 1);
        }
    } // namespace
    
    PageTable::PageTable()
        : root(make_l1())
    {
    }

    PageTable::~PageTable()
    {
        destroy_l1(root);
    }

    PageTable::PageTable(const PageTable &other)
        : root(other.root)
    {
        ++root->refcount;
    }

    PageTable &PageTable::operator=(const PageTable &other)
    {
        if (this != &other)
        {
            destroy_l1(root);
            root = other.root;
            ++root->refcount;
        }
        return *this;
    }

    PageTable::PageTable(PageTable &&other)
        : PageTable()
    {
        std::swap(root, other.root);
    }

    PageTable &PageTable::operator=(PageTable &&other)
    {
        std::swap(root, other.root);
        return *this;
    }

    uint32_t PageTable::get_page_read(uint32_t address) const
    {
        L2Table *l2 = root->tables[l1_index(address)];
        if (!l2) return PAGE_ID_UNUSED;

        LeafEntry *leaf = l2->entries[l2_index(address)];
        if (!leaf) return PAGE_ID_UNUSED;

        return leaf->page_ids[leaf_index(address)];
    }

    uint32_t PageTable::get_page_write(uint32_t address)
    {
        if (root->refcount > 1) root = replace_l1(root);

        L2Table *&l2 = root->tables[l1_index(address)];
        if (!l2) return PAGE_ID_UNUSED;
        if (l2->refcount > 1) l2 = replace_l2(l2);

        LeafEntry *&leaf = l2->entries[l2_index(address)];
        if (!leaf) return PAGE_ID_UNUSED;
        if (leaf->refcount > 1) leaf = replace_leaf(leaf);

        return leaf->page_ids[leaf_index(address)];
    }

    void PageTable::set_page(uint32_t address, uint32_t page_id)
    {
        if (root->refcount > 1) root = replace_l1(root);

        L2Table *&l2 = root->tables[l1_index(address)];
        if (!l2) l2 = make_l2();
        if (l2->refcount > 1) l2 = replace_l2(l2);

        LeafEntry *&leaf = l2->entries[l2_index(address)];
        if (!leaf && page_id == PAGE_ID_UNUSED) return;
        if (!leaf) leaf = make_leaf();
        if (leaf->refcount > 1) leaf = replace_leaf(leaf);

        if (leaf->page_ids[leaf_index(address)] != PAGE_ID_UNUSED)
        {
            // Free the old page
            page_manager.free_page(leaf->page_ids[leaf_index(address)]);
        }

        leaf->page_ids[leaf_index(address)] = page_id;

        if (page_id == PAGE_ID_UNUSED)
        {
            // Check if we can free the leaf and potentially the L2 table
            if (memcmp(leaf->page_ids, LEAF_EMPTY_ARRAY.data(), sizeof(LEAF_EMPTY_ARRAY)) == 0)
            {
                // Free the leaf
                destroy_leaf(leaf);
                leaf = nullptr;

                // Check if we can free the L2 table
                if (memcmp(l2->entries, L2_EMPTY_ARRAY, sizeof(L2_EMPTY_ARRAY)) == 0)
                {
                    destroy_l2(l2);
                    l2 = nullptr;
                }
            }
        }
    }

    void PageTable::clear()
    {
        destroy_l1(root);
        root = make_l1();
    }

    PageTable::L1Table *PageTable::make_l1(L1Table *old_l1)
    {
        L1Table *new_l1 = alloc<L1Table>();
        if (old_l1)
        {
            for (size_t i = 0; i < L1_NUM_ENTRIES; ++i)
            {
                // copy l2 tables
                new_l1->tables[i] = old_l1->tables[i];

                // increment reference counts
                if (new_l1->tables[i]) ++new_l1->tables[i]->refcount;
            }
        }
        else
        {
            memset(new_l1->tables, 0, sizeof(new_l1->tables));
        }

        new_l1->refcount = 1;

        return new_l1;
    }

    PageTable::L2Table *PageTable::make_l2(L2Table *old_l2)
    {
        L2Table *new_l2 = alloc<L2Table>();
        if (old_l2)
        {
            for (size_t i = 0; i < L2_NUM_ENTRIES; ++i)
            {
                // copy leaf entries
                new_l2->entries[i] = old_l2->entries[i];

                // increment reference counts
                if (new_l2->entries[i]) ++new_l2->entries[i]->refcount;
            }
        }
        else
        {
            memset(new_l2->entries, 0, sizeof(new_l2->entries));
        }

        new_l2->refcount = 1;

        return new_l2;
    }

    PageTable::LeafEntry *PageTable::make_leaf(LeafEntry *old_leaf)
    {
        LeafEntry *new_leaf = alloc<LeafEntry>();
        if (old_leaf)
        {
            for (size_t i = 0; i < LEAF_NUM_PAGES; ++i)
            {
                // Make COW pages, but preserve PAGE_ID_UNUSED
                if (old_leaf->page_ids[i] != PAGE_ID_UNUSED)
                    new_leaf->page_ids[i] = page_manager.copy(old_leaf->page_ids[i]);
                else
                    new_leaf->page_ids[i] = PAGE_ID_UNUSED;
            }
        }
        else
        {
            // Mark all pages unused
            memcpy(new_leaf->page_ids, LEAF_EMPTY_ARRAY.data(), sizeof(LEAF_EMPTY_ARRAY));
        }

        new_leaf->refcount = 1;

        return new_leaf;
    }

    PageTable::L1Table *PageTable::replace_l1(L1Table *old_l1)
    {
        L1Table *new_l1 = make_l1(old_l1);
        destroy_l1(old_l1);
        return new_l1;
    }

    PageTable::L2Table *PageTable::replace_l2(L2Table *old_l2)
    {
        L2Table *new_l2 = make_l2(old_l2);
        destroy_l2(old_l2);
        return new_l2;
    }

    PageTable::LeafEntry *PageTable::replace_leaf(LeafEntry *old_leaf)
    {
        LeafEntry *new_leaf = make_leaf(old_leaf);
        destroy_leaf(old_leaf);
        return new_leaf;
    }

    void PageTable::destroy_l1(L1Table *l1)
    {
        if (--l1->refcount > 0) return;

        for (size_t i = 0; i < L1_NUM_ENTRIES; ++i)
        {
            if (l1->tables[i]) destroy_l2(l1->tables[i]);
        }
        dealloc(l1);
    }

    void PageTable::destroy_l2(L2Table *l2)
    {
        if (--l2->refcount > 0) return;

        for (size_t i = 0; i < L2_NUM_ENTRIES; ++i)
        {
            if (l2->entries[i]) destroy_leaf(l2->entries[i]);
        }
        dealloc(l2);
    }

    void PageTable::destroy_leaf(LeafEntry *leaf)
    {
        if (--leaf->refcount > 0) return;
        for (uint32_t page_id : leaf->page_ids)
            if (page_id != PAGE_ID_UNUSED)
                page_manager.free_page(page_id);
        dealloc(leaf);
    }
} // namespace Hamster


