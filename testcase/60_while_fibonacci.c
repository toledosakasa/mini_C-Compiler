int putint(int i);
int getint();
int putchar(int a);
int n;
int fib(int p){
	int a;
	int b;
	int c;
	a = 0;
	b = 1;
	if ( p == 0 ){
		return 0;
	}
	if ( p == 1 ){
		return 1;
	}
	while ( p > 1 ){
		c = a + b;
		a = b;
		b = c;
		p = p - 1;
	}
	return c;
}
int main(){
	n = getint();
	int res;
	res = fib( n );
    int tmp;
    tmp = putint(res);
    tmp = 10;
    tmp = putchar(tmp);
	return res;
}
