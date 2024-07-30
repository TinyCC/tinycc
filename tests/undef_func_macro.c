int main () {
// This used to evaluate to 0 (0), which is invalid.
// Now it should evaluate to 0.
#if WUMBOED(WUMBO)
#endif

// Just trying a more complicated test case.
#if WUMBO && defined(WUMBOLOGY) || WUMBOED(WUMBO) && !WUMBOLOGY
    return 0;
#elif WUMBO && defined(WUMBOLOGY) || WUMBOED(WUMBO) && !WUMBOLOGY
    return 1;
#endif
}
