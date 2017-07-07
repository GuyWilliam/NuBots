#!/bin/bash

ret=0

# Loop through all c/cpp files
find . -type f \( -name *.h -o -name *.c -o -name *.cc -o -name *.cxx -o -name *.cpp -o -name *.hpp -o -name *.ipp \) -print0 | while IFS= read -r -d $'\0' line; do

    # Get what our formatted code should be
    fmt=$( clang-format-4.0 -style=file $line )

    # Check if our text is formatted incorrectly
    if ! cmp -s $line <(echo "$fmt"); then

        # Flag that it is wrong and print the difference
        ret=1
        echo "$line is incorrectly formatted"
        colordiff $line <(echo "$fmt")
    fi
done

# If we failed somewhere this will exit 1 and fail travis
exit $ret
