#define FUN 0
#define OBJ 1

#define CHA 2
#define INT 3
#define FLO 4
#define PTR 5

typedef struct {
//	char *name; //seems useless for now
//	list members;
} class_t;

typedef struct {
//	char 			*name; //seems useless for now
	unsigned char 	ret; // type de retour
//	list 			params;
} function_t;

typedef struct {
	unsigned char t; // type

	union {
		/* liste chainee ou autre en fonction du type */
		class_t c;
		function_t f;
	};
} type_t;


