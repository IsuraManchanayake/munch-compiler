void print_token(Token token) {
    switch (token.type) {
    case TOKEN_INT:
        printf("TOKEN: TOKEN_INT %llu\n", token.intval);
        break;
    case TOKEN_FLOAT:
        printf("TOKEN: TOKEN_FLOAT %f\n", token.floatval);
        break;
    case TOKEN_STR:
        printf("TOKEN: TOKEN_STR %s\n", token.strval);
        break;
    case TOKEN_NAME:
        printf("TOKEN: TOKEN_NAME %s %0X\n", token.name, (int)token.name);
        break;
    default:
        printf("TOKEN: %c\n", token.type);
    }
}
