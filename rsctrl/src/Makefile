
EXEC = protoc
#PROTO = core.proto peers.proto system.proto chat.proto search.proto files.proto stream.proto  gxs.proto  msgs.proto 
PROTO = core.proto peers.proto system.proto chat.proto search.proto files.proto stream.proto

PROTOPATH = ./definition
#CDESTPATH = ./gencc
CDESTPATH = ../../retroshare-nogui/src/rpc/proto/gencc
PYDESTPATH = ./genpy
#PYDESTPATH = ../../../../github/pyrs/pyrs/proto

CLIST = $(PROTO:%.proto=%.cc)
CCODE = $(patsubst %.proto,$(CDESTPATH)/%.pb.cc, $(PROTO))
HCODE = $(patsubst %.proto,$(CDESTPATH)/%.pb.h, $(PROTO))
PYCODE = $(patsubst %.proto,$(PYDESTPATH)/%_pb2.py, $(PROTO))


all: allc python_proto

allc: $(CCODE) 
	echo $(CCODE)

python_proto: $(PYCODE)
	echo $(PYCODE)

$(CDESTPATH)/%.pb.cc : $(PROTOPATH)/%.proto
	$(EXEC) --proto_path=$(PROTOPATH) --cpp_out=$(CDESTPATH) $<

$(PYDESTPATH)/%_pb2.py : $(PROTOPATH)/%.proto
	$(EXEC) --proto_path=$(PROTOPATH) --python_out=$(PYDESTPATH) $<

clean:
	-/bin/rm $(CCODE) $(HCODE) $(PYCODE)
