#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include "apps.h"
int req_main(int argc, char **argv);
int x509_main(int argc, char **argv);
int asn1parse_main(int argc, char **argv);

void print_usage()
{
    printf("Usage\n");
    printf(" ./openssl req  : Generate a CSR (Certificate Signing Request)\n");
    printf(" ./openssl x509 : Generating a Self-Signed Certificate\n");
    printf(" ./openssl asn1pars : The asn1parse command is a diagnostic utility that can parse ASN.1 structures.\n");
    printf(" ./openssl -h   : Help\n");
    printf("\nCommand Help\n");
    printf(" Eg: ./openssl req -h\n");
}
int main(int argc, char **argv)
{
    
    if(argc == 1) {
        print_usage();
        return 0;
    }
    argc--;
    argv++;
    if(strcmp(argv[0],"req")==0)
        req_main(argc,argv);
    else
        if(strcmp(argv[0],"x509")==0)
            x509_main(argc,argv);
        else if(strcmp(argv[0],"asn1pars")==0)
            asn1parse_main(argc,argv);
        else
        {   
            if(strcmp(argv[0],"-h")!=0)
                printf("\nInvalid command:\n");
            print_usage();
        }
    return 0;
}
