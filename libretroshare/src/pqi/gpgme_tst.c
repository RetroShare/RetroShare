
#include "pqi/authgpg.h"

const std::string key_path("./tmp/privkey.pem");
const std::string passwd("8764");
const std::string gpg_passwd("aaaa");
const std::string name("Test X509");
const std::string email("test@email.com");
const std::string org("Org");
const std::string loc("Loc");
const std::string state("State");
const std::string country("GB");

int main()
{
	/* Init the auth manager */

	GPGAuthMgr mgr;


	/* Select which GPG Keys we use */

	/* print all keys */
	mgr.printKeys();

	std::list<std::string> idList;
	mgr.availablePGPCertificates(idList);

	if (idList.size() < 1)
	{
		fprintf(stderr, "No GPG Certificate to use!\n");
		exit(1);
	}
	std::string id = idList.front();
	fprintf(stderr, "Using GPG Certificate:%s \n", id.c_str());

 	std::string noname;
	mgr.GPGInit(id);
	mgr.LoadGPGPassword(gpg_passwd);

	/* Init SSL library */
	mgr.InitAuth(NULL, NULL, NULL);

	/* then try to generate and sign a X509 certificate */
        int nbits_in = 2048;
	std::string errString;

	/* Generate a Certificate Request */
	X509_REQ *req = GenerateX509Req(key_path, passwd, name, email, org,
			loc, state, country, nbits_in, errString);

	        // setup output.
        BIO *bio_out = NULL;
        bio_out = BIO_new(BIO_s_file());
        BIO_set_fp(bio_out,stdout,BIO_NOCLOSE);

	/* Print it out */
	int nmflag = 0;
	int reqflag = 0;

        X509_REQ_print_ex(bio_out, req, nmflag, reqflag);

	X509 *x509 = mgr.SignX509Req(req, 100, gpg_passwd);

	X509_print_ex(bio_out, x509, nmflag, reqflag);

	BIO_flush(bio_out);
	BIO_free(bio_out);

	/* now try to validate it */
	mgr.AuthX509(x509);

	//sleep(10);
}


