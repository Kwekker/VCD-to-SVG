#!/bin/bash

# Declare an associative array for HTML escape codes
declare -A html_escape=(
    ['"']='&quot;'
    ["'"]='&apos;'
    ['&']='&amp;'
    ['<']='&lt;'
    ['>']='&gt;'
)

# Loop through all printable ASCII characters from 32 to 126
for ((i=32; i<=126; i++)); do
    char=$(printf "\\$(printf '%03o' $i)")  # Convert ASCII number to character

    # Check if the character is in the html_escape array
    if [[ -v "html_escape[$char]" ]]; then
        escaped_char="${html_escape[$char]}"   # Replace with escape code
    else
        escaped_char="$char"
    fi

    ./measure.sh "11$escaped_char 11"              # Run the command with the character
done
