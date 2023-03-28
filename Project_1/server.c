#define _POSIX_C_SOURCE 200809L
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdbool.h>

int bufSize(char *buf, int bufSize)
{
    int returnSize = 0;
    char *temp = buf;
    while (temp < buf + bufSize)
    {
        if (*temp != '\0')
            returnSize++;
        temp++;
    }
    return returnSize;
}

bool validate(char *buf, int bufSize)
{
    int iterations = bufSize;
    char *temp = buf;

    // Jeśli ciąg znaków zaczyna się od '+' lub '-', zwróć false
    if (*temp == 43 || *temp == 45)
        return false;

    // Sprawdzanie czy ostatni znak to '\n' oraz jeśli tak, to czy przedostatni znak to '\r'
    if (*(temp + bufSize - 1) == 10)
    {
        iterations--;
        if (*(temp + bufSize - 2) == 13)
            iterations--;
    }

    // Sprawdzanie czy znaki są cyframi lub działaniami dodawania lub odejmowania
    while (temp < buf + iterations - 1)
    {
        if ((*temp < 48 || *temp > 57) && (*temp != 43 && *temp != 45))
            return false;
        if ((*temp == 43 || *temp == 45) && (*(temp + 1) == 43 || *(temp + 1) == 45))
            return false;
        temp++;
    }

    if (*temp < 48 || *temp > 57)
        return false;

    return true;
}

long int calculate(char *buf, int bufSize)
{
    long int result = 0;
    char liczba[11];
    char znak[1];
    int i = 0;
    bool add = true;
    char *temp = buf;
    while (temp < buf + bufSize)
    {
        if (*temp >= 48 && *temp <= 57)
        {
            if (i >= 10)
            {
                printf("\nBŁĄD! Przepełnienie liczby!\n");
                return 0;
            }
            znak = *temp;
            printf("\nZnak: %c", znak[0]);
            strcat(liczba, znak);
            i++;
        }
        if (*temp == 43 || *temp == 45)
        {
            if (add)
            {
                result += atol(liczba);
            }
            else
            {
                result -= atol(liczba);
            }
            liczba[0] = '\0';
            i = 0;
            add = ((*temp == 43) ? true : false);
        }
        temp++;
    }
    return result;
}

int main(int argc, char const *argv[])
{
    if (argc != 2)
    {
        perror("Zła ilość argumentów!\nPodaj numer portu jako argument!");
        return -1;
    }
    int sock;
    int rc;      // "rc" to skrót słów "result code"
    ssize_t cnt; // na wyniki zwracane przez recvfrom() i sendto()

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1)
    {
        perror("socket");
        return 1;
    }

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_addr = {.s_addr = htonl(INADDR_ANY)},
        .sin_port = htons(atoi(argv[1]))};

    rc = bind(sock, (struct sockaddr *)&addr, sizeof(addr));
    if (rc == -1)
    {
        perror("bind");
        return 1;
    }

    unsigned char buf[65528];
    long int result;

    bool keep_on_handling_clients = true;
    while (keep_on_handling_clients)
    {
        struct sockaddr_in clnt_addr;
        socklen_t clnt_addr_len;

        clnt_addr_len = sizeof(clnt_addr);

        cnt = recvfrom(sock, buf, 65528, 0, (struct sockaddr_in *)&clnt_addr, &clnt_addr_len);
        if (cnt == -1)
        {
            perror("recvfrom");
            return 1;
        }
        printf("\nOtrzymano działanie: %s = ", buf);
        result = calculate(buf, sizeof(buf));
        printf("%ld\n\n", result);

        sprintf(buf, "%ld", result);

        cnt = sendto(sock, buf, sizeof(buf), 0, (struct sockaddr *)&clnt_addr, clnt_addr_len);
        if (cnt == -1)
        {
            perror("sendto");
            return 1;
        }
        printf("Sent %zi bytes\n", cnt);
    }

    rc = close(sock);
    if (rc == -1)
    {
        perror("close");
        return 1;
    }

    return 0;
}