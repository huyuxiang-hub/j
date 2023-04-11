#!/usr/bin/env python
"""
mtds.py : comparing S4RandomArray between event pairs
========================================================


"""
import os, numpy as np
from opticks.ana.fold import Fold


if __name__ == '__main__':
    a = Fold.Load("$AFOLD", symbol="a")
    b = Fold.Load("$BFOLD", symbol="b")

    print(repr(a))
    print(repr(b))

    aa = a.S4RandomArray.reshape(-1)  
    bb = b.S4RandomArray.reshape(-1)  
    


