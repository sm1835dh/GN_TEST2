int gt_compute_track_similarity_prim_params(gt_track_t *trk1,
                                            gt_track_t *trk2,
                                            int flags,
                                            int verbose,
                                            const gt_sim_params_t *params,
                                            gt_similarity_t *sim,
                                            char *report_out,
                                            size_t size_report){
	/* Fills in the passed in similarity struct. */

	int		gid1;
	int		gid2;
	gt_goetcorr_t	goetvect1[GT_MAX_VECTORSIZE];
	int		goetvectcnt1;
	gt_goetcorr_t	goetvect2[GT_MAX_VECTORSIZE];
	int		goetvectcnt2;
	int		sum;
	int		corr;
	int     report_index;

	if(trk1 == NULL || trk2 == NULL || sim == NULL) {
		gt_log("Missing argument!");
		return EINVAL;
	}

	memset(sim, 0, sizeof(gt_similarity_t));
	sum = 0;

	if(verbose) {
		gt_log("");
		if (report_out) {
            report_index = 0;
            memset(report_out, 0, size_report);
		}
	}

	/* Step1: Primary genre cutoff. */
	gid1 = gt_get_track_primary_goet_value(trk1, GT_TYPE_GENRE);
	gid2 = gt_get_track_primary_goet_value(trk2, GT_TYPE_GENRE);
	corr = gt_get_goet_correlation(gid1, gid2);
	if(corr < params->genre_cutoff) {
		sim->gs_step1_pass = 0;

		if(verbose) {
			gt_log("STEP 1: %d (%d) %d < %d -> FAIL",
			    gid1, corr, gid2, params->genre_cutoff);

            if (report_out) {
                report_index += snprintf(report_out,
                                         size_report,
                                         "STEP 1: %d (%d) %d < %d -> FAIL\n",
                                         gid1,
                                         corr,
                                         gid2,
                                         params->genre_cutoff);
            }
		}

		if(!(flags & GT_SIMFLAG_NO_EARLY_BAILOUT))
			return 0;

	} else {

		if(verbose) {
			gt_log("STEP 1: %d (%d) %d >= %d -> Compare MOOD",
			    gid1, corr, gid2, params->genre_cutoff);

            if (report_out) {
                report_index += snprintf(report_out,
                                         size_report,
                                         "STEP 1: %d (%d) %d >= %d -> Compare MOOD",
                                         gid1,
                                         corr,
                                         gid2,
                                         params->genre_cutoff);
            }
		}

		/* Step1: Primary mood cutoff. */
		gid1 = gt_get_track_primary_goet_value(trk1, GT_TYPE_MOOD);
		gid2 = gt_get_track_primary_goet_value(trk2, GT_TYPE_MOOD);
		if(gid1 && gid2) {
			corr = gt_get_goet_correlation(gid1, gid2);

            if(corr < params->mood_cutoff) {
				sim->gs_step1_pass = 0;

				if(verbose) {
					gt_log(
					    "STEP 1: %d (%d) %d < %d -> FAIL",
					    gid1, corr, gid2,
					    params->mood_cutoff);
                    if (report_out) {
                        report_index += snprintf(report_out + report_index,
                                                 size_report - report_index,
                                                 "\nSTEP 1: %d (%d) %d < %d -> FAIL\n",
                                                 gid1,
                                                 corr,
                                                 gid2,
                                                 params->mood_cutoff);
                    }
				}

				if(!(flags & GT_SIMFLAG_NO_EARLY_BAILOUT))
					return 0;
			} else {
				sim->gs_step1_pass = 1;

				if(verbose) {
					gt_log(
					    "STEP 1: %d (%d) %d >= %d -> PASS",
					    gid1, corr, gid2, params->mood_cutoff);

                    if (report_out) {
                        report_index += snprintf(report_out + report_index,
                                                 size_report - report_index,
                                                 "\nSTEP 1: %d (%d) %d >= %d -> PASS",
                                                 gid1,
                                                 corr,
                                                 gid2,
                                                 params->mood_cutoff);
                    }
				}
			}
		}
	}

	if(verbose) {
		gt_log("");
		gt_log("STEP 2: (Genre=%d Mood=%d Era=%d Origin=%d"
		    " Artist Type=%d)\n",
		    params->genre_weight, params->mood_weight, params->era_weight,
		    params->origin_weight, params->atype_weight);

        if (report_out) {
            report_index += snprintf(report_out + report_index,
                                     size_report - report_index,
                                     "\n\nSTEP 2: (Genre=%d Mood=%d Era=%d Origin=%d"
                                     " Artist Type=%d)\n",
                                     params->genre_weight,
                                     params->mood_weight,
                                     params->era_weight,
                                     params->origin_weight,
                                     params->atype_weight);
        }
	}

	/* Genre. */
	goetvectcnt1 = gt_get_track_goet_vector(trk1, GT_TYPE_GENRE, goetvect1,
	    GT_MAX_VECTORSIZE);
	goetvectcnt2 = gt_get_track_goet_vector(trk2, GT_TYPE_GENRE, goetvect2,
	    GT_MAX_VECTORSIZE);
	if(goetvectcnt1 <= 0 || goetvectcnt2 <= 0)
		sim->gs_score_genre = 0;
	else {
		if(verbose) {
			gt_log("Computing GENRE similarity:");
			if (report_out) {
                report_index += snprintf(report_out + report_index, size_report - report_index, "\nComputing GENRE similarity:");
			}
		}
		sim->gs_score_genre = gt_compute_vector_similarity(
		    goetvect1, goetvectcnt1, goetvect2, goetvectcnt2, 0,
		    verbose, report_out, size_report);
		if(verbose) {
			gt_log("");
			if (report_out) {
                report_index = strlen(report_out);
                report_index += snprintf(report_out + report_index, size_report - report_index, "\n");
			}
		}
		/* Only account for genre weight if present for both. */
		sum += params->genre_weight;
	}

	/* Mood. */
	goetvectcnt1 = gt_get_track_goet_vector(trk1, GT_TYPE_MOOD, goetvect1,
	    GT_MAX_VECTORSIZE);
	goetvectcnt2 = gt_get_track_goet_vector(trk2, GT_TYPE_MOOD, goetvect2,
	    GT_MAX_VECTORSIZE);
	if(goetvectcnt1 <= 0 || goetvectcnt2 <= 0)
		sim->gs_score_mood = DEFAULT_MOOD_SCORE;
	else {
		if(verbose) {
			gt_log("Computing MOOD similarity:");
			if (report_out) {
                report_index += snprintf(report_out + report_index, size_report - report_index, "\nComputing MOOD similarity:");
			}
		}
		sim->gs_score_mood = gt_compute_vector_similarity(
		    goetvect1, goetvectcnt1, goetvect2, goetvectcnt2, 1,
		    verbose, report_out, size_report);
		if(verbose) {
			gt_log("");
			if (report_out) {
                report_index = strlen(report_out);
                report_index += snprintf(report_out + report_index, size_report - report_index, "\n");
			}
		}
		/* Only account for mood weight if present for both. */
		sum += params->mood_weight;
	}

	/* Era. */
	goetvectcnt1 = gt_get_track_goet_vector(trk1, GT_TYPE_ERA, goetvect1,
	    GT_MAX_VECTORSIZE);
	goetvectcnt2 = gt_get_track_goet_vector(trk2, GT_TYPE_ERA, goetvect2,
	    GT_MAX_VECTORSIZE);
	if(goetvectcnt1 <= 0 || goetvectcnt2 <= 0)
		sim->gs_score_era = 0;
	else {
		if(verbose) {
			gt_log("Computing ERA similarity:");
			if (report_out) {
                report_index += snprintf(report_out + report_index, size_report - report_index, "\nComputing ERA similarity:");
			}
		}
		sim->gs_score_era = gt_compute_vector_similarity(
		    goetvect1, goetvectcnt1, goetvect2, goetvectcnt2, 0,
		    verbose, report_out, size_report);
		if(verbose) {
			gt_log("");
			if (report_out) {
                report_index = strlen(report_out);
                report_index += snprintf(report_out + report_index, size_report - report_index, "\n");
			}
		}
		/* Only account for era weight if present for both. */
		sum += params->era_weight;
	}


	/* Origin. */
	goetvectcnt1 = gt_get_track_goet_vector(trk1, GT_TYPE_ORIGIN,
	    goetvect1, GT_MAX_VECTORSIZE);
	goetvectcnt2 = gt_get_track_goet_vector(trk2, GT_TYPE_ORIGIN,
	    goetvect2, GT_MAX_VECTORSIZE);
	if(goetvectcnt1 <= 0 || goetvectcnt2 <= 0)
		sim->gs_score_origin = 0;
	else {
		if(verbose) {
			gt_log("Computing ORIGIN similarity:");
			if (report_out) {
                report_index += snprintf(report_out + report_index, size_report - report_index, "\nComputing ORIGIN similarity:");
			}
		}
		sim->gs_score_origin = gt_compute_vector_similarity(
		    goetvect1, goetvectcnt1, goetvect2, goetvectcnt2, 0,
		    verbose, report_out, size_report);
		if(verbose) {
			gt_log("");
			if (report_out) {
                report_index = strlen(report_out);
                report_index += snprintf(report_out + report_index, size_report - report_index, "\n");
			}
		}
		/* Only account for origin weight if present for both. */
		sum += params->origin_weight;
	}

	/* Artist type. */
	goetvectcnt1 = gt_get_track_goet_vector(trk1, GT_TYPE_ATYPE,
	    goetvect1, GT_MAX_VECTORSIZE);
	goetvectcnt2 = gt_get_track_goet_vector(trk2, GT_TYPE_ATYPE,
	    goetvect2, GT_MAX_VECTORSIZE);
	if(goetvectcnt1 <= 0 || goetvectcnt2 <= 0)
		sim->gs_score_artist_type = 0;
	else {
		if(verbose) {
			gt_log("Computing ARTIST TYPE similarity:");
			if (report_out) {
                report_index += snprintf(report_out + report_index, size_report - report_index, "\nComputing ARTIST TYPE similarity:");
			}
		}
		sim->gs_score_artist_type = gt_compute_vector_similarity(
		    goetvect1, goetvectcnt1, goetvect2, goetvectcnt2, 0,
		    verbose, report_out, size_report);
		if(verbose) {
			gt_log("");
			if (report_out) {
                report_index = strlen(report_out);
                report_index += snprintf(report_out + report_index, size_report - report_index, "\n");
			}
		}
		/* Only account for artist type weight if present for both. */
		sum += params->atype_weight;
	}

	/* Overall score is a weighted sum of the components. */
	sim->gs_score_weighted = (
	    sim->gs_score_genre * params->genre_weight +
	    sim->gs_score_mood * params->mood_weight +
	    sim->gs_score_era * params->era_weight +
	    sim->gs_score_origin * params->origin_weight +
	    sim->gs_score_artist_type * params->atype_weight) / sum;

	if(sim->gs_step1_pass)
		sim->gs_similarity_value = sim->gs_score_weighted;

	return 0;
}
