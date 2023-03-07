# #!/bin/bash
# gcc -o enc_server enc_server.c
# gcc -o enc_client enc_client.c
# gcc -o dec_server dec_server.c
# gcc -o dec_client dec_client.c
# gcc -o keygen keygen.c

keygen: keygen.c otp.c
	gcc -o keygen keygen.c

enc_server: enc_server.c otp.c dialog.c
	gcc -o enc_server enc_server.c

enc_client: enc_client.c otp.c dialog.c
	gcc -o enc_client enc_client.c

dec_server: dec_server.c otp.c dialog.c
	gcc -o dec_server dec_server.c

dec_client: dec_client.c otp.c dialog.c
	gcc -o dec_client dec_client.c

clean:
	rm -f keygen enc_server enc_client dec_server dec_client

compileall: keygen enc_server enc_client dec_server dec_client

