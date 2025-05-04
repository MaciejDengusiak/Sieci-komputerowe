#!/bin/bash

# Porównanie plików
if diff ../lecture_code/lecture_plik1 ../Solution3/moj_plik1 > /dev/null; then
    echo "Pliki są identyczne ✅"
else
    echo "Pliki się różnią ❌"
fi
