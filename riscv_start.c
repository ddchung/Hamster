
extern int main(void);
extern void _exit(int status);

__attribute__((naked))
void _start(void)
{
    _exit(main());
}
