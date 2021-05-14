%define api.prefix {yini_}

%union {
    int token;
    int i;
    struct string *s;
    struct ini_entry *entry;
    struct ini_value value;
    entry_list *entries;
    value_list *list;
}

%code requires {
    #include "kvec.h"
    #include "system4/ini.h"

    kv_decl(entry_list, struct ini_entry*);
    kv_decl(value_list, struct ini_value);
    extern entry_list *yini_entries;
}

%{

#include <stdio.h>
#include "system4.h"
#include "system4/string.h"

entry_list *yini_entries;
extern unsigned long yini_line;
extern int yini_lex();
void yini_error(const char *s) { ERROR("at line %lu: %s", yini_line, s); }

static entry_list *make_ini(void)
{
    entry_list *ini = xmalloc(sizeof(entry_list));
    kv_init(*ini);
    return ini;
}

static void push_entry(entry_list *ini, struct ini_entry *entry)
{
    kv_push(struct ini_entry*, *ini, entry);
}

static struct ini_value make_list(value_list *values)
{
    if (!values)
	return ini_make_list(NULL, 0);
    struct ini_value *vals = kv_data(*values);
    size_t nr_vals = kv_size(*values);
    free(values);
    return ini_make_list(vals, nr_vals);
}

static value_list *make_value_list(void)
{
    value_list *values = xmalloc(sizeof(value_list));
    kv_init(*values);
    return values;
}

static void push_value(value_list *values, struct ini_value value)
{
    kv_push(struct ini_value, *values, value);
}

static struct ini_value make_entry_value(value_list *values)
{
    struct ini_value *vals = kv_data(*values);
    size_t nr_vals = kv_size(*values);
    free(values);

    if (nr_vals == 1) {
	struct ini_value val = vals[0];
	free(vals);
	return val;
    }
    return ini_make_list(vals, nr_vals);
}

static struct ini_entry *make_list_assign(struct string *name, int i, value_list *values)
{
    struct ini_value *val = xmalloc(sizeof(struct ini_value));
    *val = make_entry_value(values);

    struct ini_value wrapper = {
	.type = _INI_LIST_ENTRY,
	._list_pos = i,
	._list_value = val
    };
    return ini_make_entry(name, wrapper);
}

static struct ini_entry *make_entry(struct string *name, value_list *values)
{
    return ini_make_entry(name, make_entry_value(values));
}

static struct ini_entry *make_formation(struct string *name, entry_list *entries)
{
    struct ini_value wrapper = {
	.type = INI_FORMATION,
	.nr_entries = kv_size(*entries),
	.entries = xcalloc(kv_size(*entries), sizeof(struct ini_entry)),
    };
    for (size_t i = 0; i < wrapper.nr_entries; i++) {
	wrapper.entries[i] = *kv_A(*entries, i);
	free(kv_A(*entries, i));
    }
    kv_destroy(*entries);
    free(entries);
    return ini_make_entry(name, wrapper);
}

%}

%token	<token>		TRUE FALSE FORMATION
%token	<i>		INTEGER
%token	<s>		STRING IDENTIFIER

%type	<entries>	inifile
%type	<entries>	entries
%type	<entry>		entry
%type	<value>		value
%type	<list>		list

%start inifile

%%

inifile	: 	entries { yini_entries = $1; }
	;

entries	:	entry         { $$ = make_ini(); push_entry($$, $1); }
	|	entries entry { push_entry($1, $2); }
	;

entry	:	IDENTIFIER                 '=' list { $$ = make_entry($1, $3); }
	|	IDENTIFIER '[' INTEGER ']' '=' list { $$ = make_list_assign($1, $3, $6); }
	|	FORMATION STRING '{' entries '}'    { $$ = make_formation($2, $4); }
	;

value	:	INTEGER          { $$ = ini_make_integer($1); }
	|	TRUE             { $$ = ini_make_boolean(true); }
	|	FALSE            { $$ = ini_make_boolean(false); }
	|	STRING           { $$ = ini_make_string($1); }
	|	'{' '}'          { $$ = make_list(NULL); }
	|	'{' list '}'     { $$ = make_list($2); }
	|	'{' list ',' '}' { $$ = make_list($2); }
	;

list	:	value          { $$ = make_value_list(); push_value($$, $1); }
	|	list ',' value { push_value($1, $3); }
	;

%%
