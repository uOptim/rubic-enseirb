/* Type */
#define FUN   0
#define CLA   1
#define OBJ   2

#define INT   3
#define FLO   4
#define STR   5

typedef struct {
	char    *name;   // usefull to know the name of a super class
//	class_t *super;
//	list    members;
} class_t;

typedef struct {
	char    *cn; // object class name
//	class_t *cp; // might be usefull one day
} object_t;

typedef struct {
	unsigned char ret; // return type 
	char          *cn; // class name is needed when return type is OBJ 
//	list   params;
} function_t;

typedef struct {
	union {
		unsigned char t;         // general type
		struct {
			unsigned char tt:7;  // specific type
			unsigned char tc:1;  // is const
		};
	};

	union {
		/* liste chainee ou autre en fonction du type */
		class_t    c;
		object_t   o;
		function_t f;
	};
} type_t;


