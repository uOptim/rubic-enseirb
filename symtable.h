#define FUN 0
#define OBJ 1

#define CHA 2
#define INT 3
#define FLO 4
#define PTR 5

typedef struct {
	char *name;
//	list members;
} class_t;

typedef struct {
	char *name;
//	list params;
} function_t;

struct type {
	unsigned char t; // type

	union {
		/* liste chainee ou autre en fonction du type */
		class_t c;
		function_t f;
	};
};


