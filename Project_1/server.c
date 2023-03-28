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
#include <limits.h>

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

int calculate(char *buf, int bufSize, bool **errorStream)
{
    int result = 0;
    long long int number = 0;
    char liczba[20];
    memset(liczba, 0, sizeof(liczba));
    char znak[2];
    memset(znak, 0, sizeof(znak));
    bool add = true;
    char *temp = buf;
    while (temp < buf + bufSize)
    {
        while (*temp != 43 && *temp != 45 && temp < buf + bufSize)
        {
            znak[0] = *temp;
            strcat(liczba, znak);
            temp++;
        }

        if (add)
            number += atoll(liczba);
        else
            number -= atoll(liczba);

        memset(liczba, 0, sizeof(liczba));

        if (temp < buf + bufSize)
        {
            add = (*temp == 43) ? true : false;
        }

        temp++;
    }

    if (number > INT_MAX || number < INT_MIN)
    {
        *errorStream = (bool *)true;
        return -1;
    }
    result = (int)number;
    return result;
}

int main(int argc, char const *argv[])
{
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
        .sin_port = htons(2020)};

    rc = bind(sock, (struct sockaddr *)&addr, sizeof(addr));
    if (rc == -1)
    {
        perror("bind");
        return 1;
    }

    unsigned char buf[65536];
    memset(buf, 0, sizeof(buf));
    int result;
    bool *errorStream = false;

    bool keep_on_handling_clients = true;
    while (keep_on_handling_clients)
    {
        struct sockaddr_in clnt_addr;
        socklen_t clnt_addr_len;

        clnt_addr_len = sizeof(clnt_addr);

        cnt = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr_in *)&clnt_addr, &clnt_addr_len);
        if (cnt == -1)
        {
            perror("recvfrom");
            return 1;
        }
        printf("\nOtrzymano działanie: %s = ", buf);
        if (validate(buf, bufSize(buf, sizeof(buf))))
        {
            result = calculate(buf, bufSize(buf, sizeof(buf)), (bool **)&errorStream);
            if (!errorStream)
            {
                printf("%d", result);
                memset(buf, 0, sizeof(buf));
                sprintf(buf, "%d", result);
            }
            else
            {
                printf("ERROR - przepelnienie wyniku\n");
                memset(buf, 0, sizeof(buf));
                sprintf(buf, "%s", "ERROR");
                errorStream = false;
            }
        }
        else
        {
            printf("ERROR - niepoprawne dane\n");
            memset(buf, 0, sizeof(buf));
            sprintf(buf, "%s", "ERROR");
            errorStream = false;
        }

        cnt = sendto(sock, buf, bufSize(buf, sizeof(buf)), 0, (struct sockaddr *)&clnt_addr, clnt_addr_len);
        if (cnt == -1)
        {
            perror("sendto");
            return 1;
        }
        printf("\nSent %zi bytes\n\n", cnt);
    }

    rc = close(sock);
    if (rc == -1)
    {
        perror("close");
        return 1;
    }

    return 0;
}