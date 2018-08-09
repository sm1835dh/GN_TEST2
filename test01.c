int
gt_compute_vector_similarity(gt_goetcorr_t *vect1, int vectcnt1,
    gt_goetcorr_t *vect2, int vectcnt2, int include_unknown_pair, int verbose, char *report_out, size_t size_report)
{
	/* NOTE: this function modifies the vectors passed in!! */

	int		sum;
	int		weightsum;
	int		i;
	int		t;
	int 	j;
	gt_goetcorr_match_t	corrs[GT_MAX_VECTORSIZE * GT_MAX_VECTORSIZE];
	int 	num_corrs;
	int		corr;
	int		weight;
	int 	total_weight_1, total_weight_2;
	int 	squared_weight_1, squared_weight_2;
	gt_goetcorr_t	*gc1;
	gt_goetcorr_t	*gc2;
	gt_goetcorr_t 	*gcswap; // in case reordering has to happen
	int 	intswap;
	int 	swap;
	int		known;
	int		unknown;
	int		res;
	int		itercnt;
	int     report_index;

	num_corrs = vectcnt1 * vectcnt2;

    if (verbose && report_out)
        report_index = strlen(report_out);

	/* Optimization/quick check: if both vectors consist of single value
	 * at 100% weight, just return the correlation between the GOETs, no
	 * need to bother with allocations for such "single value" GOETs. */
	if(vectcnt1 == 1 && vectcnt2 == 1 &&
	    vect1[0].gc_weight == 100 && vect2[0].gc_weight == 100) {

		corr = gt_get_goet_correlation(vect1[0].gc_id, vect2[0].gc_id);

		if(verbose) {
			gt_log("  SINGLE: %u (%d) %u",
			    vect1[0].gc_id, corr, vect2[0].gc_id);

            if (report_out) {
                report_index += snprintf(report_out + report_index,
                                         size_report - report_index,
                                         "\n  SINGLE: %u (%d) %u",
                                         vect1[0].gc_id,
                                         corr,
                                         vect2[0].gc_id);
            }
		}

		return corr;
	}

	total_weight_1 = total_weight_2 = 0;
	squared_weight_1 = squared_weight_2 = 0;
	swap = 0;
	for (i = 0; i < vectcnt1; ++i)
	{
		total_weight_1 += vect1[i].gc_weight;
		squared_weight_1 += (vect1[i].gc_weight * vect1[i].gc_weight);
	}
	for (i = 0; i < vectcnt2; ++i)
	{
		total_weight_2 += vect2[i].gc_weight;
		squared_weight_2 += (vect2[i].gc_weight * vect2[i].gc_weight);
	}
	if (total_weight_1 < total_weight_2) {
		// vect1 should have the higher weight
		swap = 1;
	}
	else if (total_weight_1 == total_weight_2) {
		// vect1 should have higher statistical dispersion
		// using average distance from the mean
		if (vectcnt1 == vectcnt2) {
			// shortcut if counts are equal: greater sum of squares
			// has higher average distance from mean
			if (squared_weight_1 < squared_weight_2)
				swap = 1;
		}
		else {
			int disp1 = (vectcnt2 * vectcnt2 * vectcnt1 * squared_weight_1) +
				((vectcnt1 * vectcnt1) - (vectcnt2 * vectcnt2)) * total_weight_1 * total_weight_1;
			int disp2 = (vectcnt2 * vectcnt1 * vectcnt1 * squared_weight_2);
			if (disp1 < disp2)
				swap = 1;
		}
	}

	if (swap == 1) {
		gcswap = vect1;
		vect1 = vect2;
		vect2 = gcswap;
		intswap = vectcnt1;
		vectcnt1 = vectcnt2;
		vectcnt2 = intswap;
		intswap = total_weight_1;
		total_weight_1 = total_weight_2;
		total_weight_2 = intswap;

	}

	/* Preload the correlation scores
	 */
	j = 0;
	for(i = 0; i < vectcnt1; ++i) {
		gc1 = &vect1[i];
		for(t = 0; t < vectcnt2; ++t) {
			gc2 = &vect2[t];
			corrs[j].gm_corr = gt_get_goet_correlation(gc1->gc_id,
			    gc2->gc_id);
			corrs[j].gm_index1 = i;
			corrs[j].gm_index2 = t;
			corrs[j].gm_order = j;
			++j;
		}
	}

	qsort((void *) corrs, num_corrs, sizeof(gt_goetcorr_match_t),
			gt_compare_goetcorrs);

	sum = weightsum = 0;
	res = 0;

	itercnt = 0;
	while (total_weight_1 && total_weight_2 && itercnt < num_corrs) {
		gc1 = &vect1[corrs[itercnt].gm_index1];
		gc2 = &vect2[corrs[itercnt].gm_index2];

		weight = (gc1->gc_weight < gc2->gc_weight) ?
			gc1->gc_weight : gc2->gc_weight;
		if (weight > 0) {
			if(verbose) {
				gt_log("  %d. %u/%d (%d) %u/%d",
				    itercnt, gc1->gc_id, gc1->gc_weight,
				    corrs[itercnt].gm_corr, gc2->gc_id, gc2->gc_weight);

	            if (report_out) {
	                report_index += snprintf(report_out + report_index,
	                                         size_report - report_index,
	                                         "\n  %d. %u/%d (%d) %u/%d",
	                                         itercnt,
	                                         gc1->gc_id,
	                                         gc1->gc_weight,
	                                         corrs[itercnt].gm_corr,
	                                         gc2->gc_id,
	                                         gc2->gc_weight);
	            }
			}

			gc1->gc_weight -= weight;
			gc2->gc_weight -= weight;
			total_weight_1 -= weight;
			total_weight_2 -= weight;

			sum += weight * corrs[itercnt].gm_corr;
			weightsum += weight;

			if(verbose) {
				gt_log("     Sum=%d, Weightsum=%d",
				    sum, weightsum);
	            if (report_out) {
	                report_index += snprintf(report_out + report_index,
	                                         size_report - report_index,
	                                         "\n     Sum=%d, Weightsum=%d",
	                                         sum,
	                                         weightsum);
	            }
			}
		}
		++itercnt;

	}

	res = 0;
	if(weightsum > 0) {
		known = sum / weightsum;

		if(verbose) {
			gt_log("   --------------------");
			gt_log("   Known = %d / %d", known, weightsum);

			if (report_out) {
                report_index += snprintf(report_out + report_index, size_report - report_index, "\n   --------------------");
                report_index += snprintf(report_out + report_index, size_report - report_index, "\n   Known = %d / %d", known, weightsum);
			}
		}

		if(!include_unknown_pair) {
			res = known;
		} else {
			unknown = known / 2;

			if(verbose) {
				gt_log(" Unknown = %d / %d",
				    unknown, 100 - weightsum);
                if (report_out) {
                    report_index += snprintf(report_out + report_index,
                                             size_report - report_index,
                                             "\n Unknown = %d / %d",
                                             unknown,
                                             100 - weightsum);
                }
			}

			res = (known * weightsum + unknown * (100 - weightsum)) / 100;
		}

		if(verbose) {
			gt_log("   ====================");
			gt_log("   Result = %d", res);

			if (report_out) {
                report_index += snprintf(report_out + report_index, size_report - report_index, "\n   ====================");
                report_index += snprintf(report_out + report_index, size_report - report_index, "\n   Result = %d", res);
			}
		}
	}

	return res;
}