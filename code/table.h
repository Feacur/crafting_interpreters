#if !defined(LOX_TABLE)
#define LOX_TABLE

#include "value.h"

struct Obj_String;

typedef struct {
	struct Obj_String * key;
	Value value;
} Entry;

typedef struct {
	uint32_t capacity, count;
	Entry * entries;
} Table;

void table_init(Table * table);
void table_free(Table * table);
bool table_get(Table * table, struct Obj_String * key, Value * value);
bool table_set(Table * table, struct Obj_String * key, Value value);
bool table_delete(Table * table, struct Obj_String * key);
void table_add_all(Table * table, Table * to);

#endif
