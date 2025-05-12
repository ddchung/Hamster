// Ram Filesystem

#pragma once

#include <filesystem/base_file.hpp>

namespace Hamster
{
    class RamFsData;

    /**
     * @brief A filesystem that is stored in RAM, and can also use swap to store files
     * @note This is NOT persistent, and data will be lost on unmount
     */
    class RamFs : public BaseFilesystem
    {
    public:
        RamFs();
        ~RamFs();

        RamFs(const RamFs &) = delete;
        RamFs &operator=(const RamFs &) = delete;

        RamFs(RamFs &&);
        RamFs &operator=(RamFs &&);

        /**
         * @brief Open the root directory of the filesystem.
         * @param flags The flags to open the directory with
         * @return A newly allocated `BaseDirectory` that operates on the root directory, or on error, it returns nullptr and sets `error`
         * @note Be sure to free the directory
         * @note This is equivelant to POSIX `open` on the filesystem's base
         * @note The filesystem's base is not necessarily the root of Hamster's FS, due to mounts
         */
        BaseDirectory *open_root(int flags) override;

    private:
        RamFsData *data;
    };
} // namespace Hamster

