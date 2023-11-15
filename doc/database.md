## Feature Requirements

1. Support incremental updating
2. Find symbol declare / implementation / reference



## Table Design



### Symbol Table

| Symbol_ID (Primary Key, INTEGER) | Symbol_Name (TEXT) | Symbol_Kind (INTEGER)                              |
| ------------------------------------ | ------------------ | -------------------------------------------------- |
| Unique identifier for each symbol. | The name of symbol | function / struct / enum / macro / global variable |



### Position Table

| Position_ID (Primary Key)           | Symbol_ID (Foreign Key)     | Position_Kind                        | File_Uri | Start_Byte | End_Byte | Start_Line | Start_Column | End_Line | End_Column |
| ----------------------------------- | --------------------------- | ------------------------------------ | -------- | ---------- | -------- | ---------- | ------------ | -------- | ---------- |
| Unique identifier for each position | Symbol_ID from Symbol_Table | declare / implementation / reference |          |            |          |            |              |          |            |



### File Table

| File_Uri (Primary Key, TEXT)          | Update_Date (INTEGER)     |
| ------------------------------------- | ------------------------- |
| The file path. E.g. file:///foo/bar.c | The update date in epoch. |

