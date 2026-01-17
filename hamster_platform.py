import os

Import("env")

platform = env.GetProjectOption("hamster_platform")

pioenv = env["PIOENV"]
config = env.GetProjectConfig()

src_filter = config.get(f'env:{pioenv}', 'build_src_filter')

src_filter_copy = src_filter.copy()
for filter in src_filter_copy:
    src_filter.append(f'{filter[:2]}../port/{platform}/{filter[2:]}')

env.Replace(SRC_FILTER=src_filter)

print(f"Hamster: building $PROJECT_DIR/port/{platform}")
