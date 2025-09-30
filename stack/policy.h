#if tolerance_lvl >= 1
#warning "tolerance level is 1 or more"
#define T1(...) __VA_ARGS__ // Включены минимальные проверки, тестовая версия с минимальными накладными расходами
#else
#define T1(...)
#endif

#if tolerance_lvl >= 2 // Включено больше проверок
#warning "tolerance level is 2 or more"
#define T2(...) __VA_ARGS__
#else
#define T2(...)
#endif

#if tolerance_lvl >= 3 // Максимальные проверки
#warning "tolerance level is 3 or more"
#define T3(...) __VA_ARGS__
#else
#define T3(...)
#endif
