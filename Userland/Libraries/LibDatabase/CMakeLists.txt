set(SOURCES
	AbstractTableProvider.cpp
	CSVTableProvider.cpp
	Table.cpp
)

serenity_lib(LibDatabase database)
target_link_libraries(LibDatabase)
