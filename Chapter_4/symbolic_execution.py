#!/usr/bin/env python3
#-*- coding: UTF-8 -*-
import sys
from maat import *

# First we create a symbolic engine for our platform
engine = MaatEngine(ARCH.X64, OS.LINUX)

# now we load the binary, we need to give the type of
# the binary
engine.load("./example-symbolic-execution", BIN.ELF64, args=[], base=0x00100000, load_interp=False)

# let's going to create symbolic memory
# for the addresses where our data would
# be stored on the stack
engine.mem.make_symbolic(engine.cpu.rbp.as_uint()-0x8, 1, 4, "arg1")
engine.mem.make_symbolic(engine.cpu.rbp.as_uint()-0x4, 1, 4, "arg2")

def exec_callback(m: MaatEngine):
    # method just to print the executed address
    print(f"Exec instruction at {m.info.addr}")

def find_values(m: MaatEngine):
    '''
    Method that will check if the branch is the one we want
    and will inject the conditions in the solver that
    will retrieve the values we can use in the program to reach
    the different parts of the code.
    '''
    if m.info.addr == 0x001011b3: # care only about the first branch
        s = Solver()
        print("Adding the branch condition in case is taken (numbers are not multiples)")
        print(f"condition: {m.info.branch.cond}")
        s.add(m.info.branch.cond)
        if s.check():
            model = s.get_model()
            print(f"Found a model for branch:")
            print(f"arg1 = {model.get('arg1_0')}")
            print(f"arg2 = {model.get('arg2_0')}")
        else:
            print("Not found a model...")
        s = Solver()
        print("Adding the invert of the branch condition (numbers are multiples)")
        print(f"condition: {m.info.branch.cond.invert()}")
        s.add(m.info.branch.cond.invert())
        arg1 = m.mem.read(engine.cpu.rbp.as_uint()-0x8, 4)
        arg2 = m.mem.read(engine.cpu.rbp.as_uint()-0x4, 4)
        s.add(arg1 != 0)
        s.add(arg2 != 0)
        if s.check():
            model = s.get_model()
            print(f"Found a model for branch:")
            print(f"arg1 = {model.get('arg1_0')}")
            print(f"arg2 = {model.get('arg2_0')}")
        else:
            print("Not found a model...")
        return ACTION.HALT

# insert the callbacks as hooks for different events
engine.hooks.add(EVENT.EXEC, WHEN.BEFORE, filter=(0x00101169, 0x001011d5), callbacks=[exec_callback])
engine.hooks.add(EVENT.PATH, WHEN.BEFORE, callbacks=[find_values])

# run from the point where we already executed
# the atoi functions.
engine.run_from(0x001011a8)