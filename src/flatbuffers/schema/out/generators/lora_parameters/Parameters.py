# automatically generated by the FlatBuffers compiler, do not modify

# namespace: lora_parameters

import flatbuffers
from flatbuffers.compat import import_numpy
np = import_numpy()

class Parameters(object):
    __slots__ = ['_tab']

    @classmethod
    def GetRootAs(cls, buf, offset=0):
        n = flatbuffers.encode.Get(flatbuffers.packer.uoffset, buf, offset)
        x = Parameters()
        x.Init(buf, n + offset)
        return x

    @classmethod
    def GetRootAsParameters(cls, buf, offset=0):
        """This method is deprecated. Please switch to GetRootAs."""
        return cls.GetRootAs(buf, offset)
    @classmethod
    def ParametersBufferHasIdentifier(cls, buf, offset, size_prefixed=False):
        return flatbuffers.util.BufferHasIdentifier(buf, offset, b"\x47\x41\x49\x4C", size_prefixed=size_prefixed)

    # Parameters
    def Init(self, buf, pos):
        self._tab = flatbuffers.table.Table(buf, pos)

    # Parameters
    def Parameters(self, j):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(4))
        if o != 0:
            x = self._tab.Vector(o)
            x += flatbuffers.number_types.UOffsetTFlags.py_type(j) * 4
            x = self._tab.Indirect(x)
            from Generators.lora_parameters.Tensor import Tensor
            obj = Tensor()
            obj.Init(self._tab.Bytes, x)
            return obj
        return None

    # Parameters
    def ParametersLength(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(4))
        if o != 0:
            return self._tab.VectorLen(o)
        return 0

    # Parameters
    def ParametersIsNone(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(4))
        return o == 0

def ParametersStart(builder):
    builder.StartObject(1)

def Start(builder):
    ParametersStart(builder)

def ParametersAddParameters(builder, parameters):
    builder.PrependUOffsetTRelativeSlot(0, flatbuffers.number_types.UOffsetTFlags.py_type(parameters), 0)

def AddParameters(builder, parameters):
    ParametersAddParameters(builder, parameters)

def ParametersStartParametersVector(builder, numElems):
    return builder.StartVector(4, numElems, 4)

def StartParametersVector(builder, numElems: int) -> int:
    return ParametersStartParametersVector(builder, numElems)

def ParametersEnd(builder):
    return builder.EndObject()

def End(builder):
    return ParametersEnd(builder)
