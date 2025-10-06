#!/bin/bash
# This script is used to build the Hamster project, without any specific build system.

command="$1"
if [[ -z "$command" ]]; then
    echo "Usage: $0 <command>"
    echo "Available commands: build, run, help"
    exit 1
fi

if [[ "$command" == "build" ]]; then
    echo "Building the Hamster project..."

    # Check for g++
    if ! command -v g++ &> /dev/null; then
        echo "Error: g++ is not installed. Please install it to build the project."
        exit 1
    fi

    if [[ ! -d "src" ]]; then
        echo "Error: Source directory 'src' not found. Please ensure you are in the project root."
        exit 1
    fi

    mkdir -p build


    # Clean up any previous build artifacts
    rm -rf build/* 2>/dev/null
    rm hamster 2>/dev/null

    # Find the C++ source files in the src directory
    files=$(find src -type f -name "*.cpp")

    echo "Compiling the following files:"

    for file in $files; do
        printf "$file "
    done

    echo

    for file in $files; do
        if [[ ! -f "$file" ]]; then
            echo "Error: Source file '$file' not found."
            exit 1
        fi

        echo "Compiling $file..."

        output_file="build/$file.o"

        mkdir -p "$(dirname "$output_file")"

        g++ -std=c++20 -Wall -Wextra -Wno-maybe-musttail-local-addr -Wno-unused-parameter -Wno-unused-function -Ofast -march=native -mtune=native -funroll-loops \
            -Isrc -Iinclude -c "$file" -o "$output_file" &
    done

    wait

    echo "Linking object files..."

    to_link=$(find build -type f -name "*.o")

    g++ -std=c++20 -Wall -Wextra -Ofast -march=native -mtune=native -funroll-loops \
        -o hamster $to_link
    
    if [[ $? -ne 0 ]]; then
        echo "Error: Build failed. Please check the output for details."
        exit 1
    fi
    echo "Build completed successfully. You can now run the project using './hamster', or '$0 run'."
elif [[ "$command" == "run" ]]; then
    echo "Running the Hamster project..."
    if [[ ! -x "hamster" ]]; then
        echo "Error: Hamster executable not found. Please build the project first."
        exit 1
    fi
    exec ./hamster
else
    echo "Unknown command: $command"
    echo "Available commands: build, run, help"
    echo "build - Builds, but doesn't run the project."
    echo "run - Runs the project after building. Does not rebuild"
    echo "help - Displays this help message."
    exit 1
fi
