int putint(int i);
int getint();
int putchar(int a);
int a[10];
int func(int p){
	p = p - 1;
	return p;
}
int main(){
	int b;
    b = 10;
	b = func(b+10);
    putint(b);
	return b;
}
