// Copyright (C) 2003-2005 RiO
//
// cg_hud.h -- HUD headers

#define MAX_HUDELEMENTS 1024
#define MAX_HUDCHILDREN   10
#define MAX_HUDSTRING	 255

typedef enum {
	HUD_IMAGE,
	HUD_LABEL,
	HUD_VAR
} HUD_ElemType_t;

typedef struct HUD_Image_s {
	int			x, y;
	qhandle_t	source;
} HUD_Image_t;

typedef struct HUD_Label_s {
	int			x, y;
	char		text[MAX_HUDSTRING];
} HUD_Label_t;

typedef struct HUD_Var_s {
	void		*value;
} HUD_Var_t;

typedef struct HUD_Elem_s {
	HUD_ElemType_t	type;

	struct HUD_Elem_s *child[MAX_HUDCHILDREN];
	struct HUD_Elem_s *parent;

	union {
		HUD_Image_t	image;
		HUD_Label_t	label;
		HUD_Var_t	var;
	};

} HUD_Elem_t;