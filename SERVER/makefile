SRC_DIR :=./
CC:=g++
LIBS:= -lprotobuf -lpthread -lmysqlclient

CPP_FILE:=$(wildcard ./*.cc)
OBJS:=$(patsubst %.cc,%.o,$(CPP_FILE))
objs:=$(addsuffix /*.o, $(SRC_DIR))
TARGET := a.out

$(TARGET):$(OBJS)
	$(CC) ../ProtobufMsg/Message.pb.cc -o $@ $^ $(LIBS)
%.o:%.cpp
	$(CC) -o $@ -c $<  $(LIBS)

object:
	@echo $(OBJS)
clean:
	rm -rf $(TARGET) $(OBJS)
rebuild: clean $(TARGET)
