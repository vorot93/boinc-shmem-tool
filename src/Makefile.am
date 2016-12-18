NULL =

bin_PROGRAMS = boinc-shmem-tool
boinc_shmem_tool_SOURCES = \
    main.cpp \
    $(NULL)
boinc_shmem_tool_CXXFLAGS = \
    $(AM_CXXFLAGS) \
    $(APP_API_CFLAGS) \
    $(JSONCPP_CFLAGS) \
    $(NULL)
boinc_shmem_tool_LDADD = \
    $(AM_LDFLAGS) \
    -lpthread \
    $(APP_API_LIBS) \
    $(JSONCPP_LIBS) \
    $(NULL)
