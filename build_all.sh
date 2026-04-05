#!/bin/bash
cd /mnt/c/Users/Nospace/PLD_COMP
mkdir -p build_artifacts

echo " Compiling all test files "
for file in testfiles/*.c; do
    name=$(basename "$file" .c)
    echo "Compiling $name..."
    python3 ifcc-test.py -S -o "build_artifacts/$name.s" "$file"
    if [ $? -eq 0 ]; then
        echo "✓ $name compiled to assembly successfully"
    else
        echo "✗ Failed to compile $name"
    fi
done

echo ""
echo " Artifacts created "
ls -lh build_artifacts/ | tail -20
echo ""
echo "Total files:"
ls build_artifacts/ | wc -l
