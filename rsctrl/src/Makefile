
EXEC = protoc
#PROTO = base.proto  files.proto  gxs.proto  msgs.proto  peers.proto  system.proto
PROTO = base.proto peers.proto

PROTOPATH = ./definition
CDESTPATH = ./gencc
#CDESTPATH = ../../retroshare-nogui/src/rpc/proto/gencc
PYDESTPATH = ./genpy

CLIST = $(PROTO:%.proto=%.cc)
CCODE = $(patsubst %.proto,$(CDESTPATH)/%.pb.cc, $(PROTO))
HCODE = $(patsubst %.proto,$(CDESTPATH)/%.pb.h, $(PROTO))
PYCODE = $(patsubst %.proto,$(PYDESTPATH)/%_pb2.py, $(PROTO))


all: allc
	echo $(CCODE)
	echo $(PYCODE)

allc: $(CCODE) $(PYCODE)

$(CDESTPATH)/%.pb.cc : $(PROTOPATH)/%.proto
	$(EXEC) --proto_path=$(PROTOPATH) --cpp_out=$(CDESTPATH) $<

$(PYDESTPATH)/%_pb2.py : $(PROTOPATH)/%.proto
	$(EXEC) --proto_path=$(PROTOPATH) --python_out=$(PYDESTPATH) $<

clean:
	-/bin/rm $(CCODE) $(HCODE) $(PYCODE)
