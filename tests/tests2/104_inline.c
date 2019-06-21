inline void inline_inline_2decl_only(void);
inline void inline_inline_2decl_only(void);

inline void inline_inline_undeclared(void){}

inline void inline_inline_predeclared(void);
inline void inline_inline_predeclared(void){}

inline void inline_inline_postdeclared(void){}
inline void inline_inline_postdeclared(void);

inline void inline_inline_prepostdeclared(void);
inline void inline_inline_prepostdeclared(void){}
inline void inline_inline_prepostdeclared(void);

inline void inline_inline_undeclared2(void){}

inline void inline_inline_predeclared2(void);
inline void inline_inline_predeclared2(void);
inline void inline_inline_predeclared2(void){}

inline void inline_inline_postdeclared2(void){}
inline void inline_inline_postdeclared2(void);
inline void inline_inline_postdeclared2(void);

inline void inline_inline_prepostdeclared2(void);
inline void inline_inline_prepostdeclared2(void);
inline void inline_inline_prepostdeclared2(void){}
inline void inline_inline_prepostdeclared2(void);
inline void inline_inline_prepostdeclared2(void);

extern void extern_extern_undeclared(void){}

extern void extern_extern_predeclared(void);
extern void extern_extern_predeclared(void){}

extern void extern_extern_postdeclared(void){}
extern void extern_extern_postdeclared(void);

extern void extern_extern_prepostdeclared(void);
extern void extern_extern_prepostdeclared(void){}
extern void extern_extern_prepostdeclared(void);

extern void extern_extern_undeclared2(void){}

extern void extern_extern_predeclared2(void);
extern void extern_extern_predeclared2(void);
extern void extern_extern_predeclared2(void){}

extern void extern_extern_postdeclared2(void){}
extern void extern_extern_postdeclared2(void);
extern void extern_extern_postdeclared2(void);

extern void extern_extern_prepostdeclared2(void);
extern void extern_extern_prepostdeclared2(void);
extern void extern_extern_prepostdeclared2(void){}
extern void extern_extern_prepostdeclared2(void);
extern void extern_extern_prepostdeclared2(void);

void extern_undeclared(void){}

void extern_predeclared(void);
void extern_predeclared(void){}

void extern_postdeclared(void){}
void extern_postdeclared(void);

void extern_prepostdeclared(void);
void extern_prepostdeclared(void){}
void extern_prepostdeclared(void);

void extern_undeclared2(void){}

void extern_predeclared2(void);
void extern_predeclared2(void);
void extern_predeclared2(void){}

void extern_postdeclared2(void){}
void extern_postdeclared2(void);
void extern_postdeclared2(void);


extern inline void noinst_extern_inline_undeclared(void){}

extern inline void noinst_extern_inline_postdeclared(void){}
inline void noinst_extern_inline_postdeclared(void);

extern inline void noinst_extern_inline_postdeclared2(void){}
inline void noinst_extern_inline_postdeclared2(void);
inline void noinst_extern_inline_postdeclared2(void);

extern inline void inst_extern_inline_postdeclared(void){}
extern inline void inst_extern_inline_postdeclared(void);
inline void inst2_extern_inline_postdeclared(void){}
void inst2_extern_inline_postdeclared(void);

void inst_extern_inline_predeclared(void);
extern inline void inst_extern_inline_predeclared(void){}
void inst2_extern_inline_predeclared(void);
inline void inst2_extern_inline_predeclared(void){}
extern inline void inst3_extern_inline_predeclared(void);
inline void inst3_extern_inline_predeclared(void){}

static inline void noinst_static_inline_postdeclared(void){}
static inline void noinst_static_inline_postdeclared(void);
static inline void noinst2_static_inline_postdeclared(void){}
static void noinst2_static_inline_postdeclared(void);

static void noinst_static_inline_predeclared(void);
static inline void noinst_static_inline_predeclared(void){}
static void noinst2_static_inline_predeclared(void);
static inline void noinst2_static_inline_predeclared(void){}

static void static_func(void);
void static_func(void) { }

inline void noinst_extern_inline_func(void);
void noinst_extern_inline_func(void) { }

int main()
{
        inline_inline_undeclared(); inline_inline_predeclared(); inline_inline_postdeclared();
        inline_inline_undeclared2(); inline_inline_predeclared2(); inline_inline_postdeclared2();
	noinst_static_inline_predeclared();
	noinst2_static_inline_predeclared();
	noinst_static_inline_predeclared();
	noinst2_static_inline_predeclared();

        void check_exports();
        check_exports();
        return 0;
}
