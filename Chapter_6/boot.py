from avatar2 import * 
from avatar2.peripherals import *
from types import SimpleNamespace



class UARPrf(AvatarPeripheral):
    def hw_read(self, offset, size): 
        if offset == 0x18: 
            return self.status         
        return 0


    def hw_write(self, offset, size, value): 
        if offset == 0: 
            sys.stderr.write(chr(value & 0xff))
            sys.stderr.flush()
        else:
            self.log_write(value, size, "UARTWRITE")
        return True

    def __init__(self, name, address, size, **kwargs): 
        super().__init__(name, address, size, **kwargs)
        self.status = 0 
        self.read_handler[0:size] = self.hw_read  
        self.write_handler[0:size] = self.hw_write 


def fake_signal_handler(*args, **kwargs):
    pass

from pandare import Panda

Panda.setup_internal_signal_handler = fake_signal_handler
Panda._setup_internal_signal_handler = fake_signal_handler



avatar = Avatar(arch=ARM, cpu_model='cortex-r7') 
emu = avatar.add_target(PyPandaTarget, entry_address=0x4000000e) 

 
entries = [{ 

"load_address":0x40010000,
"size":0x25479a0 ,
"file":"modem_main.bin",
"name":"MAIN", 
}
,
{"load_address":0x40000000,
"size": 0x1E40,
"file":"boot.bin",
"name":"BOOT", 
},

{"load_address":0x47EE0000,
"size": 0x100000,
"file":"nv.bin",
"name":"NV", 
}



] 
 

avatar.add_memory_range(0x84000000, 0x1000, name='logging-uart', emulate=UARPrf)

for e in entries:
    entry = SimpleNamespace(**e)
    avatar.add_memory_range(entry.load_address, entry.size, file=entry.file, name=entry.name, permission='rwx') 

avatar.init_targets() 
emu.cont()
emu.wait()
