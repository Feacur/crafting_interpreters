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
void table_add_all(Table * table, Table * from);

struct Obj_String * table_find_key_copy(Table * table, char const * chars, uint32_t length, uint32_t hash);
struct Obj_String * table_find_key_concatenate(Table * table, char const * a_chars, uint32_t a_length, char const * b_chars, uint32_t b_length, uint32_t hash);

void gc_mark_table_grey(Table * table);
void gc_table_remove_white_keys(Table * table);

#endif
