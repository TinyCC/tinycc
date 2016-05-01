extern void vide(void);
__asm__("vide: ret");

int main() {
    vide();
    return 0;
}
