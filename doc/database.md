## Feature Requirements

1. Incremental updating
2. Find symbol declare / implementation / reference
3. Call hierarchy
4. Call tree



## Table Design



### Symbol Table

| symbol_id (Primary Key, INTEGER) | symbol_name (TEXT) | symbol_kind (INTEGER)                              |
| ------------------------------------ | ------------------ | -------------------------------------------------- |
| Unique identifier for each symbol. | The name of symbol | function / field / type / global variable |

>+ `function`: It is a function.
>+ `field`: It is a field of `struct` or `enum` or `class`.
>+ `type`: It is an alias of other type, or a `struct` or an `enum` or a `class`.
>+ `gvar`: It is a global variable.



### Position Table

| position_id (Primary Key, INTEGER)  | symbol_id (Foreign Key)     | position_kind (INTEGER)         | file_id (Foreign Key) | start_byte (INTEGER) | end_byte (INTEGER) | start_line (INTEGER) | start_column (INTEGER) | end_line (INTEGER) | end_column (INTEGER) |
| ----------------------------------- | --------------------------- | ------------------------------- | --------------------- | -------------------- | ------------------ | -------------------- | ---------------------- | ------------------ | -------------------- |
| Unique identifier for each position | Symbol_ID from Symbol_Table | declare / implementation / xref |                       |                      |                    |                      |                        |                    |                      |



### File Table

| file_id (Primary Key, INTEGER) | file_uri (TEXT, unique)               | update_date (INTEGER)     |
| ------------------------------ | ------------------------------------- | ------------------------- |
| Unique identifier for file.    | The file path. E.g. file:///foo/bar.c | The update date in epoch. |



### XREF Table



| ref_id (Primary Key, INTEGER)     | ref_src (Foreign Key (symbol_id)) | ref_dst (Foreign Key (symbol_id)) | position_id (Foreign Key) | ref_kind     |
| --------------------------------- | --------------------------------- | --------------------------------- | ------------------------- | ------------ |
| Unique identifier for each record | The symbol                        | The symbol that use the `xref`    |                           | alias / call |



> example:
>
> ```c
> typedef int foo_t;
> ```
>
> 
