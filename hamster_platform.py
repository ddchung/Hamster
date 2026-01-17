import os

Import("env")

platform = env.GetProjectOption("hamster_platform")

pioenv = env["PIOENV"]
config = env.GetProjectConfig()
project_dir = env.subst("$PROJECT_DIR")

src_filter = [ f'{filter[:2]}../port/{platform}/{filter[2:]}' for filter in config.get(f'env:{pioenv}', 'build_src_filter') ]

env.AppendUnique(SRC_FILTER=src_filter)
env.Append(CCFLAGS=[f'-I{project_dir}/port/{platform}'])

print(f"Hamster: building {project_dir}/port/{platform}")
