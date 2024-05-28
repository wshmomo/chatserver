#定义一个SRC_LIST变量，包含了该目录下的所有源文件
aux_source_directory(. SRC_LIST)

# aux_source_directory(../server/model MODEL_LIST)

#指定生成可执行文件
add_executable(ChatClient ${SRC_LIST})

#指定可执行文件链接时需要依赖的库文件
target_link_libraries(ChatClient pthread)

#如果在server文档中已经构建了user.cpp，我们需要把这个链接上,这里我进行修改不太行，然后我就直接在include文件夹下的model文件夹下的user.hpp进行了修改，并把相应地user.cpp删掉了