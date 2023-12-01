#include <string>

#include <openssl/ssl.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/aes.h>
#include <openssl/rand.h>

std::string encryptAES(const std::string &plaintext, const std::string &key);

/* Implementation so smol stay here? or move to new cpp file? */

std::string encryptAES(const std::string &plaintext, const std::string &key)
{
	EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
	if (!ctx)
	{
		// Handle error: Unable to create the EVP_CIPHER_CTX
		return "";
	}

	// Assuming key is 32 bytes for AES-256-CBC
	if (key.size() != 32)
	{
		// Handle error: Key size must be 32 bytes for AES-256-CBC
		EVP_CIPHER_CTX_free(ctx);
		return "";
	}

	// Generate a random initialization vector
	unsigned char iv[AES_BLOCK_SIZE];
	RAND_bytes(iv, AES_BLOCK_SIZE);

	// Initialize encryption operation
	if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, reinterpret_cast<const unsigned char *>(key.c_str()), iv) != 1)
	{
		// Handle error: Unable to initialize encryption
		EVP_CIPHER_CTX_free(ctx);
		return "";
	}

	std::vector<unsigned char> ciphertext(plaintext.size() + AES_BLOCK_SIZE);
	int len = 0;
	int ciphertext_len = 0;

	// Encrypt the plaintext
	if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len, reinterpret_cast<const unsigned char *>(plaintext.c_str()), plaintext.size()) != 1)
	{
		// Handle error: Encryption failed
		EVP_CIPHER_CTX_free(ctx);
		return "";
	}
	ciphertext_len += len;

	// Finalize the encryption
	if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len) != 1)
	{
		// Handle error: Finalizing encryption failed
		EVP_CIPHER_CTX_free(ctx);
		return "";
	}
	ciphertext_len += len;

	// Clean up
	EVP_CIPHER_CTX_free(ctx);

	// Convert encrypted data to a string (note: it may contain non-printable characters)
	return std::string(reinterpret_cast<char *>(ciphertext.data()), ciphertext_len);
}