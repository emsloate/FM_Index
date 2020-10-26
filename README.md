# FM_Index
Partial implementation of FM Index, using external BWT transformation of input string to perform backwards search  - CMSC858D.

Using [this](https://github.com/toiletpapar/BWT) implementation of BWT. To compile, you must use the [rank_support](https://github.com/emsloate/rank_support) class as well as the BWT.cpp file form the above repo (in the folder BWT here). ```./bwocc``` by itself will run benchmark tests. To create and save an FM-Index, run ```./bwocc build <input string file> <output file>```, where ```<input string file>``` contains the reference string. To load and use this to search a list of queries, run ```./bwocc query <saved fmindex> <query patterns>```, where  ```<saved fmindex>``` is the same as ```<output file>``` from the build command, and ```<query patterns>``` is a text file where each line is a query.

