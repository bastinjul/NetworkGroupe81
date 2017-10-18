#include "packet_interface.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <zlib.h>


struct __attribute__((__packed__)) pkt {
	ptypes_t type : 2;
	uint8_t tr : 1;
	uint8_t window : 5;
	uint8_t seqnum;
	uint16_t length;
	uint32_t timestamp;
	uint32_t crc1;
	char* payload;
	uint32_t crc2;
};

/* Extra code */
/* Your code will be inserted here */

pkt_t* pkt_new()
{
	pkt_t* pkt = calloc(1, sizeof(pkt_t));
	if(pkt == NULL){
		return NULL;
	}
	return pkt;
}

void pkt_del(pkt_t *pkt)
{
    if(pkt != NULL){
			if(pkt->payload != NULL){
				free(pkt->paylaod);
			}
			free(pkt);
		}
}

/*
 * Decode des donnees recues et cree une nouvelle structure pkt.
 * Le paquet recu est en network byte-order.
 * La fonction verifie que:
 * - Le CRC32 du header recu est le même que celui decode a la fin
 *   du header (en considerant le champ TR a 0)
 * - S'il est present, le CRC32 du payload recu est le meme que celui
 *   decode a la fin du payload
 * - Le type du paquet est valide
 * - La longueur du paquet et le champ TR sont valides et coherents
 *   avec le nombre d'octets recus.
 *
 * @data: L'ensemble d'octets constituant le paquet recu
 * @len: Le nombre de bytes recus
 * @pkt: Une struct pkt valide
 * @post: pkt est la representation du paquet recu
 *
 * @return: Un code indiquant si l'operation a reussi ou representant
 *         l'erreur rencontree.
 */
pkt_status_code pkt_decode(const char *data, const size_t len, pkt_t *pkt)
{
	// à faire
}

/*
 * Encode une struct pkt dans un buffer, prêt a être envoye sur le reseau
 * (c-a-d en network byte-order), incluant le CRC32 du header et
 * eventuellement le CRC32 du payload si celui-ci est non nul.
 *
 * @pkt: La structure a encoder
 * @buf: Le buffer dans lequel la structure sera encodee
 * @len: La taille disponible dans le buffer
 * @len-POST: Le nombre de d'octets ecrit dans le buffer
 * @return: Un code indiquant si l'operation a reussi ou E_NOMEM si
 *         le buffer est trop petit.
 */
pkt_status_code pkt_encode(const pkt_t* pkt, char *buf, size_t *len)
{
 	//à faire
}

ptypes_t pkt_get_type  (const pkt_t* pkt)
{
	return pkt->type;
}

uint8_t  pkt_get_tr(const pkt_t* pkt)
{
	return pkt->tr;
}

uint8_t  pkt_get_window(const pkt_t* pkt)
{
	return pkt->window;
}

uint8_t  pkt_get_seqnum(const pkt_t* pkt)
{
	return pkt->seqnum;
}

uint16_t pkt_get_length(const pkt_t* pkt)
{
	return pkt->length;
}

uint32_t pkt_get_timestamp   (const pkt_t* pkt)
{
	return pkt->timestamp;
}

uint32_t pkt_get_crc1   (const pkt_t* pkt)
{
	return pkt->crc1;
}

uint32_t pkt_get_crc2   (const pkt_t* pkt)
{
	return pkt->crc2;
}

const char* pkt_get_payload(const pkt_t* pkt)
{
	if(pkt->payload == NULL) return NULL;
	return pkt->payload;
}


pkt_status_code pkt_set_type(pkt_t *pkt, const ptypes_t type)
{
	if (pkt == NULL) return E_UNCONSISTENT;
	if(type != PTYPE_DATA && type != PTYPE_ACK && type != PTYPE_NACK) return E_TYPE;
	else{
		pkt->type = type;
		return PKT_OK;
	}
}

pkt_status_code pkt_set_tr(pkt_t *pkt, const uint8_t tr)
{
		if (pkt == NULL) return E_UNCONSISTENT;
	pkt->tr = tr;
	return PKT_OK;
}

pkt_status_code pkt_set_window(pkt_t *pkt, const uint8_t window)
{
	if (pkt == NULL) return E_UNCONSISTENT;
	if(window > MAX_WINDOW_SIZE) return E_WINDOW;
	else{
		pkt->window = window;
		return PKT_OK;
	}
}

pkt_status_code pkt_set_seqnum(pkt_t *pkt, const uint8_t seqnum)
{
	if (pkt == NULL) return E_UNCONSISTENT;
	pkt->seqnum = seqnum;
}

pkt_status_code pkt_set_length(pkt_t *pkt, const uint16_t length)
{
	if (pkt == NULL) return E_UNCONSISTENT;
	if(length > MAX_PAYLOAD_SIZE) return E_LENGTH;
	else{
		pkt->length = htons(length);
		return PKT_OK;
	}
}

pkt_status_code pkt_set_timestamp(pkt_t *pkt, const uint32_t timestamp)
{
	if (pkt == NULL) return E_UNCONSISTENT;
	pkt->timestamp = timestamp;
	return PKT_OK;
}

pkt_status_code pkt_set_crc1(pkt_t *pkt, const uint32_t crc1)
{
	if (pkt == NULL) return E_UNCONSISTENT;
	pkt->crc1 = crc1;
	return PKT_OK;
}

pkt_status_code pkt_set_crc2(pkt_t *pkt, const uint32_t crc2)
{
	if (pkt == NULL) return E_UNCONSISTENT;
		pkt->crc2 = crc2;
		return PKT_OK;
}

pkt_status_code pkt_set_payload(pkt_t *pkt,
							    const char *data,
								const uint16_t length)
{
	if (pkt == NULL) return E_UNCONSISTENT;
	if(data == NULL){
		pkt_set_length(pkt, 0);
		pkt->payload = NULL;
		return PKT_OK;
	}
	if(pkt->payload != NULL){
		free(pkt->paylaod);
	}
	pkt->payload = (char*)malloc(length);

	if(pkt->payload == NULL) return E_NOMEM;

	memcpy(pkt->payload, data, length);

	if(pkt_set_length(pkt, length) != PKT_OK) return E_LENGTH;

	return PKT_OK;
}
