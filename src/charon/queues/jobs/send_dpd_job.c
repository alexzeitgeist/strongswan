/**
 * @file send_dpd_job.c
 * 
 * @brief Implementation of send_dpd_job_t.
 * 
 */

/*
 * Copyright (C) 2006 Tobias Brunner, Daniel Roethlisberger
 * Hochschule fuer Technik Rapperswil
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */


#include <stdlib.h>

#include "send_dpd_job.h"

#include <sa/ike_sa.h>
#include <daemon.h>


typedef struct private_send_dpd_job_t private_send_dpd_job_t;

/**
 * Private data of an send_dpd_job_t Object
 */
struct private_send_dpd_job_t {
	/**
	 * public send_dpd_job_t interface
	 */
	send_dpd_job_t public;
	
	/**
	 * ID of the IKE_SA which the message belongs to.
	 */
	ike_sa_id_t *ike_sa_id;

	/**
	 * Logger reference.
	 */
	logger_t *logger;
};

/**
 * Implements send_dpd_job_t.get_type.
 */
static job_type_t get_type(private_send_dpd_job_t *this)
{
	return SEND_DPD;
}

/**
 * Implementation of job_t.execute. 
 */ 
static status_t execute(private_send_dpd_job_t *this)
{
	ike_sa_t *ike_sa;
	status_t status;

	this->logger->log(this->logger, CONTROL|LEVEL2, "Checking out IKE SA %lld:%lld, role %s",
			this->ike_sa_id->get_initiator_spi(this->ike_sa_id),
			this->ike_sa_id->get_responder_spi(this->ike_sa_id),
			this->ike_sa_id->is_initiator(this->ike_sa_id) ? "initiator" : "responder");
	
	status = charon->ike_sa_manager->checkout(charon->ike_sa_manager,
			this->ike_sa_id, &ike_sa);
	if (status != SUCCESS)
	{
		this->logger->log(this->logger, ERROR|LEVEL1,
				"IKE SA could not be checked out. Already deleted?");
		return DESTROY_ME;
	}

	ike_sa->send_dpd_request(ike_sa);
	this->logger->log(this->logger, CONTROL|LEVEL1,
			"DPD request packet scheduled");

	this->logger->log(this->logger, CONTROL|LEVEL2,
			"Checkin IKE SA %lld:%lld, role %s",
			this->ike_sa_id->get_initiator_spi(this->ike_sa_id),
			this->ike_sa_id->get_responder_spi(this->ike_sa_id),
			this->ike_sa_id->is_initiator(this->ike_sa_id) ? "initiator" : "responder");

	status = charon->ike_sa_manager->checkin(charon->ike_sa_manager, ike_sa);
	if (status != SUCCESS)
	{
		this->logger->log(this->logger, ERROR, "Checkin of IKE SA failed!");
	}

	return SUCCESS;
}

/**
 * Implements job_t.destroy.
 */
static void destroy(private_send_dpd_job_t *this)
{
	this->ike_sa_id->destroy(this->ike_sa_id);
	free(this);
}

/*
 * Described in header
 */
send_dpd_job_t *send_dpd_job_create(ike_sa_id_t *ike_sa_id)
{
	private_send_dpd_job_t *this = malloc_thing(private_send_dpd_job_t);
	
	/* interface functions */
	this->public.job_interface.get_type = (job_type_t (*) (job_t *)) get_type;
	this->public.job_interface.destroy = (void (*) (job_t *)) destroy;
	this->public.job_interface.execute = (status_t (*) (job_t *)) execute;
	
	/* public functions */
	this->public.destroy = (void (*)(send_dpd_job_t *)) destroy;
	
	/* private variables */
	this->ike_sa_id = ike_sa_id->clone(ike_sa_id);
	this->logger = logger_manager->get_logger(logger_manager, WORKER);

	return &(this->public);
}
