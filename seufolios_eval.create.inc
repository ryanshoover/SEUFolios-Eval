<?php 
require_once('seufolios_eval.module');
global $dept;
$dept = null;

// *****
// generates the Create a Report page
function seufolios_create_report_form($form, &$form_state) {
	
	//get wp_dept_data
	$dept_data = get_wp_dept_data();

	if($dept_data) {
		$depts[] = '--choose a department--';
		foreach($dept_data as $dept) 
			$depts[$dept->abbr] = $dept->name;
	}
			
	$form['explanation'] = array(
	  '#type' => 'item',
		'#title' => 'SEUFolios Evaluation Report Generator',
	  '#markup' => '<p>Use this form to run a new evaluation report.</p>'
	);
	
	$form['seufolios_newnode_title'] = array(
	  '#type' => 'textfield',
	  '#title' => t('Report title'),
	  '#description' => t('Title of the report to be created'),
	  '#size' => 40,
	  '#maxlength' => 120,
	  '#required' => TRUE,
	);
	
	$form['query_info'] = array(
	  '#type' => 'fieldset',
	  '#title' => t('Report settings'),
	);
	
	$form['query_info']['seufolios_newnode_dept'] = array(
     '#type' => 'select',
     '#title' => t('Department'),
     '#options' => $depts,
     '#description' => t('Choose the department that you want to run the report for.'),
	 );
	
	$startdate = array( 'year'=>date('Y', time()-7776000) , 'month'=>date('n', time()-7776000) , 'day'=>date('j', time()-7776000) ); //7776000 = 3 months
	$form['query_info']['seufolios_newnode_startdate'] = array(
	  '#type' => 'date',
	  '#title' => t('Start Date'),
	  '#description' => t('The beginning of the date range for evaluations to grab'),
	  '#default_value' => $startdate,
	  '#size' => 40,
	);
	
	$form['query_info']['seufolios_newnode_enddate'] = array(
	  '#type' => 'date',
	  '#title' => t('End Date'),
	  '#description' => t('The end of the date range for evaluations to grab'),
	  //'#default_value' => date(),
	  '#size' => 40,
	);
	
	// Submit button
	$form['submit'] = array(
	'#type' => 'submit',
	'#value' => t('Run report'),
	);
	
	/*
	$result = db_query("SELECT settings FROM {seufolios_eval_settings} WHERE name = :name AND dept = :dept", array(':name'=>'slo-ques-relationship', ':dept'=>'ENGW') );
	$data = $result->fetchObject();
	$rels = unserialize($data->settings);
	
	$ques = get_wp_question_data();
	$depts = get_wp_dept_data();
	
	$form['var_dump'] = array(
		'#type' => 'item',
		'#title' => 'SLO Ques Relationship',
		'#markup' => '<pre>' .print_r($depts, true) .'</pre>',
	);
	*/
	return $form;
}
/*
function seufolios_admin_form_alter(&$form, &$form_state, $form_id)
{
    if($form_id == 'seufolios_admin_form')
        $form['#submit'] = 'seufolios_redirect_handler';
}

function seufolios_redirect_handler($form, &$form_state)
{
    unset($_REQUEST['destination']); // this doesn't seem to work though
    unset($form['#redirect']); // i think this doesnt do anything because $form is not a reference

    $form_state['redirect'] = 'engw-reports-listing';
}
*/
//Validate settings form
function seufolios_create_report_form_validate($form, &$form_state) {
	//clear any existing errors
	drupal_get_messages('error');
	
	if(!$form_state['values']['seufolios_newnode_title']) {
		form_set_error('Report title', 'Oops. Looks like you forgot to include a title for the report');
	}
	
	//!!!! NEED TO CHECK FOR SLOS AND RELATIONSHIPS IN SETTINGS TABLE
}

//Submit settings form
function seufolios_create_report_form_submit($form, &$form_state) {
	global $dept;
	$dept = $form_state['values']['seufolios_newnode_dept'];
	$result = get_wp_evaluation_data($form_state['values']);

	$sections 	= $result['sections'];
	$questions 	= $result['questions'];
	$taxes 		= $result['taxes'];
	$evals 		= $result['evals'];

	foreach($questions as $qs) {
		foreach($qs as $q) {
			$all_questions[$q->slug] = $q;
		}
	}
	
	$eval_avgs = calcAllEvalAvgs($evals);
	$current_eval_avgs = $eval_avgs[max(array_keys($eval_avgs))];

	$slos = convertAllQuesToSLO($eval_avgs, $dept);
	
	if($dept == 'ENGW') { 			//load ENGW historical data
		$history = createENGWHistory($form_state['values']['seufolios_newnode_startdate'], $form_state['values']['seufolios_newnode_enddate']);
		if($history) $slos = $slos + $history;
	}
	
	$current_slos = $slos[max(array_keys($slos))];
	
	//******
	//Create Full Report of all results
	$body_text = loadHeader();
	
	//plot all averages
	$flotname = 'all_avgs';
	$body_text .= linegraphAvgs($slos, $flotname);
	//flot current_semester_avg
	$flotname = 'current_semester_avg';
	$body_text .= barchartAvgs($current_slos, $flotname); //create flot for current semester's data
	$body_text .= printComments($current_eval_avgs);
	//$body_text .= printSubquesSLOComp($current_eval_avgs, $current_subques_avgs);
	$body_text .= printTaxes($evals);
	$body_text .= interRaterReliability($evals);
	$body_text .= printEvalQs($sections, $questions);
	$body_text .= printEvals($evals);
	
	$node = new stdClass();
	$node->type = 'engw_test'; //!!!!hard coded to ENGW, needs dynamic
	node_object_prepare($node);
	
	$node->title    = $_POST['seufolios_newnode_title'] .' Full Report'; 
	$node->language = LANGUAGE_NONE;
  
	$node->body[$node->language][0]['value']   = $body_text;
	$node->body[$node->language][0]['summary'] = text_summary($body_text);
	$node->body[$node->language][0]['format']  = 'php_code';
	
	$path = 'content/programmatically_created_node_' . date('YmdHisu');
	$node->path = array('alias' => $path);
  
	node_save($node);
	/*
	//******
	//Create Internal Report Distrubution
	$body_text = loadHeader();
	
	//plot all averages
	$flotname = 'all_avgs';
	$body_text .= linegraphAvgs($eval_avgs, $flotname);
	
	//flot current_semester_avg
	$flotname = 'current_semester_avg';
	$body_text .= barchartAvgs($current_eval_avgs, $flotname); //create flot for current semester's data
	$body_text .= printComments($current_eval_avgs);
	$body_text .= printTaxes($evals);
	$body_text .= printEvalQs($sections, $questions);
	
	$node = new stdClass();
	$node->type = 'engw_test'; //!!!!hard coded to ENGW, needs dynamic
	node_object_prepare($node);
	
	$node->title    = $_POST['seufolios_newnode_title'] .' Internal Report'; 
	$node->language = LANGUAGE_NONE;
  
	$node->body[$node->language][0]['value']   = $body_text;
	$node->body[$node->language][0]['summary'] = text_summary($body_text);
	$node->body[$node->language][0]['format']  = 'php_code';
	
	$path = 'content/programmatically_created_node_' . date('YmdHisu');
	$node->path = array('alias' => $path);
  
	node_save($node);
	
	//******
	//Create SACS Report Distrubution
	$body_text = loadHeader();
	
	//plot all averages
	$flotname = 'all_avgs';
	$body_text .= linegraphAvgs($eval_avgs, $flotname);
	
	//flot current_semester_avg
	$flotname = 'current_semester_avg';
	$body_text .= barchartAvgs($current_eval_avgs, $flotname); //create flot for current semester's data
	$body_text .= printEvalQs($sections, $questions);
	
	$node = new stdClass();
	$node->type = 'engw_test'; //!!!!hard coded to ENGW, needs dynamic
	node_object_prepare($node);
	
	$node->title    = $_POST['seufolios_newnode_title'] .' SACS Report'; 
	$node->language = LANGUAGE_NONE;
  
	$node->body[$node->language][0]['value']   = $body_text;
	$node->body[$node->language][0]['summary'] = text_summary($body_text);
	$node->body[$node->language][0]['format']  = 'php_code';
	
	$path = 'content/programmatically_created_node_' . date('YmdHisu');
	$node->path = array('alias' => $path);
  
	node_save($node);
	*/
}


//Calculate Standard Deviation
function sd_square($x, $mean) { return pow($x - $mean,2); } 
function sd($array) { 
    
  // square root of sum of squares devided by N-1 
  return sqrt(array_sum(array_map("sd_square", $array, array_fill(0,count($array), (array_sum($array) / count($array)) ) ) ) / (count($array)) ); 
}

//Used to calculate averages and standard deviations for all evaluations
function calcAllEvalAvgs($evals) {
	$time = time();
	$date = getdate($time);
	foreach($evals as $e) {
		$e_date = getdate(strtotime($e['submittime']));		//create date array from submit timestamp
	 	if($e_date['mon']>8) $e_date['year']++; 			//creates academic years from september to august (labeled by the last year)
		$sortedEvals[$e_date['year']][] = $e;				//feed the eval into array keyed by academic year
	}
	
	foreach($sortedEvals as $year=>$se) {
		$evalAvgs[$year] = calcEvalAvgs($se);
	}
	
	return $evalAvgs;
}

function calcEvalAvgs($evals) {
	global $dept;
	$wp_ques_types = get_wp_question_data();
	$depts = get_wp_dept_data();
	
	//create question types array
	foreach($wp_ques_types as $qt) {
		$q_code = urldecode($qt->code);
		if(strpos($q_code, 'type="range"') || strpos($q_code, "type='range'")) { //see if it's an input with type=range
			$q_range_pos = strpos($q_code, 'max=');						//find the start of the line that defines the max value of the range
			$q_range_str = substr($q_code, $q_range_pos, 7);
			preg_match('/(\d+)/', $q_range_str, $matches);		//pull the number out of the string we got above: /(\d+)/ is the regex to find the number
			$ques_types[$qt->id]['slug'] = $qt->slug;
			$ques_types[$qt->id]['type'] = 'range';
			$ques_types[$qt->id]['max'] = $matches[0];
		}
	}
	
	//set up ans_avg array (average of answers)
	foreach($evals as $eval) {
		if($eval['answers']) {
			$elems_ar = explode('&', $eval['answers']); 
			
			for ($i = 0; $i < count($elems_ar); $i++) {
				  $key = strtok($elems_ar[$i], "=");
				  $key = str_replace('_input', '', $key);
				  $val = strtok("=");
				  if ($val) { 				//screens out both missing values and zeros
					  $key = urldecode($key);
					  $ans_avg[$key]['total'] = 0;
					  $ans_avg[$key]['count'] = 0;
					  $ans_avg[$key]['comments'] = array();
				  } 
			}
		}
	}
	
	//process answers into array
	foreach($evals as $eval) {
		$elems_ar = explode('&', $eval['answers']);     		// get all variables
		for ($i = 0; $i < count($elems_ar); $i++) {
			$key = strtok($elems_ar[$i], "=");
			$key = str_replace('_input', '', $key);
			$val = strtok("=");
			if ($val) {
				$key = urldecode($key);
				$val = urldecode($val);
				
				//find the question type from the key
				//loop through the dept's questions looking for the right question slug
				foreach($depts as $d) {
					if($d->abbr == $dept) {
						foreach($d->eval_sections as $section) {
							foreach($section->questions as $question) {
								if($question->slug == $key)
									$ans_avg[$key]['type'] = $question->type;
							}
						}
					}
				}
				 
				if (is_numeric($val)) { 							//populate the $ans_avg array based on whether it's a comment
					if($dept == 'ENGW') $val = get_engw_conversion($val, $eval['submittime']);
					else $val = $val /$ques_types[$ans_avg[$key]['type']]['max'] *100;		//converts the value to a percent score 
					$ans_avg[$key]['total'] += $val;
					$ans_avg[$key]['count'] ++;
					$ans_avg[$key]['values'][] = $val;
				} else {
					$ans_avg[$key]['comments'][] = stripslashes($val);
				}
			}
		}	
	}
	
	//calculate answer avg and standard deviation
	foreach($ans_avg as &$a) {
		if($a['count']) {
			$a['avg'] = $a['total']/$a['count'];
			$a['sd']  = sd($a['values']);
		}
	}	
	
	return $ans_avg;
}

//special function just for ENGW
//ENGW uses a coversion to standard essay grades (A, B, C, D, F)
function get_engw_conversion($val, $time) {
	$timestamp = strtotime($time);
	$now = 1372654800; //time from 20130701 00:00:00
	
	if($timestamp > $now) { 		//5 point scale used for ENGW entering evaluations from fall 2012 on 20130717
		switch($val) {
			case 0:
				return null;
			case 1:
				return 55;
			case 2:
				return 65;
			case 3: 
				return 75;
			case 4:
				return 85;
			case 5:
				return 95;
		}
	} else {	// 6 point scale  used for ENGW evaluations from assessment in spring 2013
		switch($val) {
			/*low range*/
			case 0:
				return null;
			case 1:
				return 55;
			case 2:
				return 55;
			case 3: 
				return 65;
			case 4:
				return 75;
			case 5:
				return 85;
			case 6:
				return 95;
			/*midrage
			case 1:
				return 55;
			case 2:
				return 63;
			case 3: 
				return 71;
			case 4:
				return 79;
			case 5:
				return 87;
			case 6:
				return 95;
				*/
			/*high range
			case 0:
				return null;
			case 1:
				return 55;
			case 2:
				return 65;
			case 3: 
				return 75;
			case 4:
				return 85;
			case 5:
				return 95;
			case 6:
				return 95;
				*/
		}
	}
}

//process multi year evals into slos
function convertAllQuesToSLO($all_evals_avgs, $dept) {
	foreach($all_evals_avgs as $year=>$eval_avgs) {
		$return[$year] = convertQuesToSLO($eval_avgs, $dept);
	}
	
	return $return;
}

// take question averages and convert them to SLO averages
function convertQuesToSLO($eval_avgs, $dept) {
	//get slos
	$result = db_query("SELECT settings FROM {seufolios_eval_settings} WHERE name = :name AND dept = :dept", array(':name'=>'slos', ':dept'=>$dept) );
	$data = $result->fetchObject();
	$slos = unserialize($data->settings);
	
	//get rels from db
	$result = db_query("SELECT settings FROM {seufolios_eval_settings} WHERE name = :name AND dept = :dept", array(':name'=>'slo-ques-relationship', ':dept'=>$dept) );
	$data = $result->fetchObject();
	$rels_raw = unserialize($data->settings);
	
	//process raw into array
	foreach($rels_raw as $rel=>$val) {
		$arr = explode('-', $rel, 3);
		$rels[$arr[1]][$arr[2]] = $val; // does array SLO -> Ques -> Value
	}

	//set up return
	foreach($rels as $slo_key=>$slo) {
		$return[$slos[$slo_key]['title']]['avg'] = 0;
	}
	
	//move evals into slos based on percentages
	foreach($rels as $slo_key=>$slo) {  				//loop through slos
		foreach($slo as $ques=>$per) {  					// loop through ques
			if($per > 0) {													//don't waste time on empty values
				foreach($eval_avgs as $key=>$eval) {  // loop through evals
					if($ques == $key) {
						$return[$slos[$slo_key]['title']]['avg'] += $eval['avg'] * ($per/100);
						$return[$slos[$slo_key]['title']]['sds'][] = $eval['sd'];
					}
				}
			}
		}
	}

	//calculate standard deviation
	foreach($return as $slo_key=>$slo) {
		if($slo['sds']) {
			$sum = 0;
			foreach($slo['sds'] as $sd) 
				$sum += $sd * $sd;
		
			$return[$slo_key]['sd'] = sqrt($sum);
			//unset($return[$slo_key]['sds']);		//not needed anymore
		}
	}

	return($return);
}

//the next two take raw question avgs and convert them to ENGW SLOs
//these should now be antiquated by the two above
function convertFromSubquestionAll($evals) {
	foreach($evals as $year=>$eval) {
		if(isset($eval['des_relevant']['total'])) $converted[$year] = convertFromSubquestion($eval);
		else $converted[$year] = $eval;
	}
	return $converted;	
}
function convertFromSubquestion($ans_avg) {
	//!!! HARD CODED !!!
	//Process subquestions into student SLOs
	$slo['write_for_situation']['total'] = $ans_avg['des_relevant']['total'] + $ans_avg['des_rhetorical']['total'] +$ans_avg['des_reading_xp']['total'] +$ans_avg['rhetsit_portfolio']['total'] +$ans_avg['rhetsit_master_genres']['total'] +$ans_avg['rhetsit_variety_genres']['total'] +$ans_avg['rhetsit_rhet_strategies']['total'];
	$slo['write_for_situation']['count'] = $ans_avg['des_relevant']['count'] + $ans_avg['des_rhetorical']['count'] +$ans_avg['des_reading_xp']['count'] +$ans_avg['rhetsit_portfolio']['count'] +$ans_avg['rhetsit_master_genres']['count'] +$ans_avg['rhetsit_variety_genres']['count'] +$ans_avg['rhetsit_rhet_strategies']['count'];
	$slo['write_for_situation']['values'] = array_merge($ans_avg['des_relevant']['values'], $ans_avg['des_rhetorical']['values'], $ans_avg['des_reading_xp']['values'], $ans_avg['rhetsit_portfolio']['values'], $ans_avg['rhetsit_master_genres']['values'], $ans_avg['rhetsit_variety_genres']['values'], $ans_avg['rhetsit_rhet_strategies']['values']);
	$slo['write_for_situation']['avg']   = 
		(0.10 * $ans_avg['des_relevant']['total']/$ans_avg['des_relevant']['count']) + 
		(0.10 * $ans_avg['des_rhetorical']['total']/$ans_avg['des_rhetorical']['count']) + 
		(0.05 * $ans_avg['des_reading_xp']['total']/$ans_avg['des_reading_xp']['count']) +
		(0.20 * $ans_avg['rhetsit_portfolio']['total']/$ans_avg['rhetsit_portfolio']['count']) +
		(0.15 * $ans_avg['rhetsit_master_genres']['total']/$ans_avg['rhetsit_master_genres']['count']) +
		(0.20 * $ans_avg['rhetsit_variety_genres']['total']/$ans_avg['rhetsit_variety_genres']['count']) +
		(0.20 * $ans_avg['rhetsit_rhet_strategies']['total']/$ans_avg['rhetsit_rhet_strategies']['count']);
	$slo['write_for_situation']['sd'] = sd($slo['write_for_situation']['values']);

	$slo['analyze_situation']['total'] = $ans_avg['sources_conversation']['total'] + $ans_avg['sources_incorporate']['total'] +$ans_avg['sources_contradictory']['total'];
	$slo['analyze_situation']['count'] = $ans_avg['sources_conversation']['count'] + $ans_avg['sources_incorporate']['count'] +$ans_avg['sources_contradictory']['count'];
	$slo['analyze_situation']['values'] = array_merge($ans_avg['sources_conversation']['values'], $ans_avg['sources_incorporate']['values'], $ans_avg['sources_contradictory']['values']);
	$slo['analyze_situation']['avg']   = 
		(0.50 * $ans_avg['sources_conversation']['total']/$ans_avg['sources_conversation']['count']) + 
		(0.25 * $ans_avg['sources_incorporate']['total']/$ans_avg['sources_incorporate']['count']) + 
		(0.25 * $ans_avg['sources_contradictory']['total']/$ans_avg['sources_contradictory']['count']);
	$slo['analyze_situation']['sd'] = sd($slo['analyze_situation']['values']);
	
	$slo['writing_process']['total'] = $ans_avg['edit_big_pic']['total'] + $ans_avg['edit_line_edits']['total'] +$ans_avg['edit_copyedits']['total'];
	$slo['writing_process']['count'] = $ans_avg['edit_big_pic']['count'] + $ans_avg['edit_line_edits']['count'] +$ans_avg['edit_copyedits']['count'];
	$slo['writing_process']['values'] = array_merge($ans_avg['edit_big_pic']['values'], $ans_avg['edit_line_edits']['values'], $ans_avg['edit_copyedits']['values']);
	$slo['writing_process']['avg']   = 
		(0.40 * $ans_avg['edit_big_pic']['total']/$ans_avg['edit_big_pic']['count']) + 
		(0.40 * $ans_avg['edit_line_edits']['total']/$ans_avg['edit_line_edits']['count']) + 
		(0.20 * $ans_avg['edit_copyedits']['total']/$ans_avg['edit_copyedits']['count']);
	$slo['writing_process']['sd'] = sd($slo['writing_process']['values']);
	
	$slo['voice']['total'] = $ans_avg['rhetsit_variety_genres']['total'] + $ans_avg['rhetsit_rhet_strategies']['total'] +$ans_avg['style_authentic_voice']['total'] +$ans_avg['style_struc_trans']['total'] +$ans_avg['style_diction']['total'];
	$slo['voice']['count'] = $ans_avg['rhetsit_variety_genres']['count'] + $ans_avg['rhetsit_rhet_strategies']['count'] +$ans_avg['style_authentic_voice']['count'] +$ans_avg['style_struc_trans']['count'] +$ans_avg['style_diction']['count'];
	$slo['voice']['values'] = array_merge($ans_avg['rhetsit_variety_genres']['values'], $ans_avg['rhetsit_rhet_strategies']['values'], $ans_avg['style_authentic_voice']['values'], $ans_avg['style_struc_trans']['values'], $ans_avg['style_diction']['values']);
	$slo['voice']['avg']   = 
		(0.20 * $ans_avg['rhetsit_variety_genres']['total']/$ans_avg['rhetsit_variety_genres']['count']) + 
		(0.20 * $ans_avg['rhetsit_rhet_strategies']['total']/$ans_avg['rhetsit_rhet_strategies']['count']) + 
		(0.20 * $ans_avg['style_authentic_voice']['total']/$ans_avg['style_authentic_voice']['count']) +
		(0.20 * $ans_avg['style_struc_trans']['total']/$ans_avg['style_struc_trans']['count']) +
		(0.20 * $ans_avg['style_diction']['total']/$ans_avg['style_diction']['count']);
	$slo['voice']['sd'] = sd($slo['voice']['values']);

	$slo['conventions']['total'] = $ans_avg['edit_line_edits']['total'] + $ans_avg['edit_copyedits']['total'] +$ans_avg['style_struc_trans']['total'] +$ans_avg['sources_incorporate']['total'];
	$slo['conventions']['count'] = $ans_avg['edit_line_edits']['count'] + $ans_avg['edit_copyedits']['count'] +$ans_avg['style_struc_trans']['count'] +$ans_avg['sources_incorporate']['count'];
	$slo['conventions']['values'] = array_merge($ans_avg['edit_line_edits']['values'], $ans_avg['edit_copyedits']['values'], $ans_avg['style_struc_trans']['values'], $ans_avg['sources_incorporate']['values']);
	$slo['conventions']['avg']   = 
		(0.30 * $ans_avg['edit_line_edits']['total']/$ans_avg['edit_line_edits']['count']) + 
		(0.50 * $ans_avg['edit_copyedits']['total']/$ans_avg['edit_copyedits']['count']) + 
		(0.10 * $ans_avg['style_struc_trans']['total']/$ans_avg['style_struc_trans']['count']) +
		(0.10 * $ans_avg['sources_incorporate']['total']/$ans_avg['sources_incorporate']['count']);
	$slo['conventions']['sd'] = sd($slo['conventions']['values']);
	
	$slo['visual_presentation']['total'] = $ans_avg['des_relevant']['total'] + $ans_avg['des_cohesive']['total'] +$ans_avg['des_rhetorical']['total'] +$ans_avg['des_reading_xp']['total'];
	$slo['visual_presentation']['count'] = $ans_avg['des_relevant']['count'] + $ans_avg['des_cohesive']['count'] +$ans_avg['des_rhetorical']['count'] +$ans_avg['des_reading_xp']['count'];
	$slo['visual_presentation']['values'] = array_merge($ans_avg['des_relevant']['values'], $ans_avg['des_cohesive']['values'], $ans_avg['des_rhetorical']['values'], $ans_avg['des_reading_xp']['values']);
	$slo['visual_presentation']['avg']   = 
		(0.20 * $ans_avg['des_relevant']['total']/$ans_avg['des_relevant']['count']) + 
		(0.20 * $ans_avg['des_cohesive']['total']/$ans_avg['des_cohesive']['count']) + 
		(0.30 * $ans_avg['des_rhetorical']['total']/$ans_avg['des_rhetorical']['count']) +
		(0.40 * $ans_avg['des_reading_xp']['total']/$ans_avg['des_reading_xp']['count']);
	$slo['visual_presentation']['sd'] = sd($slo['visual_presentation']['values']);
	
	$slo['peer_review']['total'] = $ans_avg['edit_big_pic']['total'] + $ans_avg['edit_line_edits']['total'] +$ans_avg['edit_copyedits']['total'];
	$slo['peer_review']['count'] = $ans_avg['edit_big_pic']['count'] + $ans_avg['edit_line_edits']['count'] +$ans_avg['edit_copyedits']['count'];
	$slo['peer_review']['values'] = array_merge($ans_avg['edit_big_pic']['values'], $ans_avg['edit_line_edits']['values'], $ans_avg['edit_copyedits']['values']);
	$slo['peer_review']['avg']   = 
		(0.35 * $ans_avg['edit_big_pic']['total']/$ans_avg['edit_big_pic']['count']) + 
		(0.35 * $ans_avg['edit_line_edits']['total']/$ans_avg['edit_line_edits']['count']) + 
		(0.30 * $ans_avg['edit_copyedits']['total']/$ans_avg['edit_copyedits']['count']);
	$slo['peer_review']['sd'] = sd($slo['peer_review']['values']);
	
	return $slo;
}

//print out a side by side comparison of subquestion-based SLO calculations and the direct SLO ratings
//only used in Spring 2013?
function printSubquesSLOComp($subq, $slo) {
	$result = "
	<h2>Compare New and Old Questions</h2>
	<table>
	<tr><th>SLO</td><th>New Questions</th><th>Old Questions</th><th>Difference</th></tr>
	<tr><td>Write for situation</td><td>" 	.number_format(round($subq['write_for_situation']['avg'], 2), 2) ."</td><td>" 	.number_format(round($slo['write_for_situation']['avg'], 2), 2) ."</td><td>" 	.number_format(round($subq['write_for_situation']['avg']-$slo['write_for_situation']['avg'], 2), 2) ."</td></tr>
	<tr><td>Analyze situation</td><td>" 	.number_format(round($subq['analyze_situation']['avg'], 2), 2) ."</td><td>" 	.number_format(round($slo['analyze_situation']['avg'], 2), 2) ."</td><td>" 		.number_format(round($subq['analyze_situation']['avg']-$slo['analyze_situation']['avg'], 2), 2) ."</td></tr>
	<tr><td>Writing process</td><td>" 		.number_format(round($subq['writing_process']['avg'], 2), 2) ."</td><td>" 		.number_format(round($slo['writing_process']['avg'], 2), 2) ."</td><td>" 		.number_format(round($subq['writing_process']['avg']-$slo['writing_process']['avg'], 2), 2) ."</td></tr>
	<tr><td>Voice</td><td>" 				.number_format(round($subq['voice']['avg'], 2), 2) ."</td><td>" 				.number_format(round($slo['voice']['avg'], 2), 2) ."</td><td>" 					.number_format(round($subq['voice']['avg']-$slo['voice']['avg'], 2), 2) ."</td></tr>
	<tr><td>Conventions</td><td>" 			.number_format(round($subq['conventions']['avg'], 2), 2) ."</td><td>" 			.number_format(round($slo['conventions']['avg'], 2), 2) ."</td><td>" 			.number_format(round($subq['conventions']['avg']-$slo['conventions']['avg'], 2), 2) ."</td></tr>
	<tr><td>Visual Presentation</td><td>" 	.number_format(round($subq['visual_presentation']['avg'], 2), 2) ."</td><td>" 	.number_format(round($slo['visual_presentation']['avg'], 2), 2) ."</td><td>" 	.number_format(round($subq['visual_presentation']['avg']-$slo['visual_presentation']['avg'], 2), 2) ."</td></tr>
	<tr><td>Peer Review</td><td>" 			.number_format(round($subq['peer_review']['avg'], 2), 2) ."</td><td>" 			.number_format(round($slo['peer_review']['avg'], 2), 2) ."</td><td>" 			.number_format(round($subq['peer_review']['avg']-$slo['peer_review']['avg'], 2), 2) ."</td></tr>
	</table>
	";
	
	return $result;
}

//Universal JS and CSS
function loadHeader() {
	return '
	  <style>
	  .legend table { width: auto; }
	  .alert {color: red;}
	  label {display:inline; font-weight:normal;}
	  </style>	
	  
	  <script type="text/javascript">
	  function showTooltip(x, y, contents) {
			  jQuery(\'<div id="tooltip">\' + contents + \'</div>\').css( {
				  position: "absolute",
				  display: "none",
				  top: y + 5,
				  left: x + 5,
				  border: "1px solid #fdd",
				  padding: "2px",
				  "background-color": "#fee",
				  opacity: 0.80
			  }).appendTo("body").fadeIn(200);
		  }
	  
	var previousPoint = null;
	jQuery(document).ready(function($) {
		jQuery(".flot-example").bind("plothover", function (event, pos, item) {
			if (item) {
				if (previousPoint != item.datapoint) {
					previousPoint = item.datapoint;
					//alert(series.label);
					var x = item.series.xaxis.ticks[item.dataIndex].label;
					//var x = item.datapoint[0].toFixed(2);
					var y = item.datapoint[1].toFixed(2);
					jQuery("#tooltip").remove();
					showTooltip(item.pageX, item.pageY, x + "<br>" + y);
				}
			}
			else {
				jQuery("#tooltip").remove();
				previousPoint = null;            
			}
	

		jQuery(".line-chart").bind("plothover", function (event, pos, item) {
			if (item) {
				if (previousPoint != item.datapoint) {
					previousPoint = item.datapoint;
					//alert(series.label);
					//var x = item.series.xaxis.ticks[item.dataIndex].label;
					var x = item.datapoint[0].toFixed(0);
					var y = item.datapoint[1].toFixed(2);
					jQuery("#tooltip").remove();
					showTooltip(item.pageX, item.pageY, x + "<br>" + y);
				}
			}
			else {
				jQuery("#tooltip").remove();
				previousPoint = null;            
			}
		});

	});
	</script>';	
}

//create a barchart of avgs from an array with specific format
function barchartAvgs($array, $flotname) {
 	$result = '
	  <?php
	  $' .$flotname .'_data = new flotData(array(';
	
	$i = 0;
	foreach($array as $a) {
		if ($a['avg'] > 0) {
			$result .= "array($i, " .$a['avg'] .'),'; // .',' .$a['sd'] .'),';
			$i++;
		}
	}
	
	$result .='));
	  $' .$flotname .'_data->label = t("Questions");
	  $' .$flotname .'_data->bars = new flotLine();
	  
	  $' .$flotname .'_ticks = array(';
	
	$i=0;
	foreach($array as $key=>$a) {
		if($a['avg'] > 0) {
			$result .= "array($i, array('$key')),";
			$i++;
		}
	}
	
	$result .=');
	  
	  $style = new flotStyle();
	  $style->xaxis->ticks = $' .$flotname .'_ticks;
	  $style->yaxis->ticks = 7;
	  $style->yaxis->min = 0;
	  $style->grid->labelMargin = 5;
	  $style->legend->position = "ne";
	  $style->legend->noColumns = 1;
	  $style->legend->backgroundOpacity = 0.3;
		  
	  $variables = array(
		"data" => array($' .$flotname .'_data),
		"element" => array(
		  "id" => "' .$flotname .'",
		  "class" => "flot-example",
		  "style" => "width:600px;height:400px",
		),
		"options" => $style,
		"legend" => TRUE,
		"zoom" => FALSE,
	  );
	  echo "<div id=\'' .$flotname .'_wrap\'>\n<h2>Question Averages</h2>";
	  echo theme("flot_graph", $variables);
	  ?>
	  
	  <br>
	  
	  <!--table of result-->
	  <table>
	  <tr><th>Question</th><th>Average Rating</th><th>Standard Deviation</th></tr>
		  ';
		  
	foreach($array as $key=>$a) {
		if($a['avg'] > 0) {  //was $a['count']
			$result .= "<tr><td>$key</td><td>" .number_format(round($a['avg'], 2), 2) ."</td><td>" .number_format(round($a['sd'], 2), 2) ."</td></tr>\n";
		}
	}
		  
	$result .= "
	  </table></div>";	
	  
	return $result;
}

//create a line graph of avgs from an array with specific format
function linegraphAvgs($arrays, $flotname) {
foreach($arrays as $y=>$a) {
		foreach($a as $q=>$avg) {
			if(isset($avg['avg']) && is_numeric($avg['avg'])) $graph[$q][$y] = $avg['avg'];
		}
	}
	$result = '<script type="text/javascript" src="http://seufolios.cousett.com/sites/all/modules/flot/flot/jquery.flot.selection.js"></script>';
	 
	  $result .= '
	  <div id=\'' .$flotname .'_wrap\'>
	  <h2>Historical Averages</h2>
	  <div id="' .$flotname .'" class="line-chart" style="width:600px;height:400px;"></div>
      <div id="' .$flotname .'_overview" style="width:600px;height:100px;margin-top:20px"></div>
	  <p id="choices" style="margin-top:1em;">Choose dimensions to graph:</p>
	  </div>
	  <script>
	  jQuery(function () {
    	var ' .$flotname .'_data = {';
	
	  foreach($graph as $q=>$array) {  
		$result .= "\"$q\": { 
            label: \"$q\", 
            data: [";
			
		$i = 0;
		foreach($array as $y=>$avg) {
			$result .= "[$y, $avg],"; 
			$i++;
		}
		$result = substr($result, 0, -1); //kill the last comma
		$result .="] },\n";
	  }
	
	$result = substr($result, 0, -1);
	$result .= "};\n";  
	  
    $result .= '
	// hard-code color indices to prevent them from shifting as
    // countries are turned on/off
    var i = 0;
    jQuery.each(' .$flotname .'_data, function(key, val) {
        val.color = i;
        ++i;
    });  
	
	// insert checkboxes 
    var choiceContainer = jQuery("#choices");
    jQuery.each(' .$flotname .'_data, function(key, val) {
        choiceContainer.append(\'<br/><input type="checkbox" name="\' + key + \'" checked="checked" id="id\' + key + \'">&nbsp;<label for="id\' + key + \'">\' + val.label + \'</label>\');
    });
    choiceContainer.find("input").click(plotAccordingToChoices);
	
	//return the data from the checkboxes
	function getData() {
        var data = [];

        choiceContainer.find("input:checked").each(function () {
            var key = jQuery(this).attr("name");
            if (key && all_avgs_data[key])
                data.push(all_avgs_data[key]);
        });

       return data;
	}
	
	//connect the two graphs
    jQuery("#' .$flotname .'").bind("plotselected", function (event, ranges) {
        // clamp the zooming to prevent eternal zoom
        if (ranges.xaxis.to - ranges.xaxis.from < 0.00001)
            ranges.xaxis.to = ranges.xaxis.from + 0.00001;
        if (ranges.yaxis.to - ranges.yaxis.from < 0.00001)
            ranges.yaxis.to = ranges.yaxis.from + 0.00001;
        
        // do the zooming
        plot = jQuery.plot("#all_avgs", data,
                          jQuery.extend(true, {}, all_avgs_main_options, {
                              xaxis: { min: ranges.xaxis.from, max: ranges.xaxis.to }
                          }));
        
        // no fire event on the overview to prevent eternal loop
        ' .$flotname .'_overview.setSelection(ranges, true);
    });
    jQuery("#' .$flotname .'_overview").bind("plotselected", function (event, ranges) {
        all_avgs_main.setSelection(ranges);
    });
	

    function plotAccordingToChoices() {
        data = getData();

		all_avgs_main_options = {
			xaxis: { tickDecimals: 0 },
			grid:  { backgroundColor: "#f8f8f8", hoverable: true, borderWidth: 0 },
			selection: { mode: "x" }
		};

        if (data.length > 0)
            ' .$flotname .'_main = jQuery.plot(jQuery("#' .$flotname .'"), data, all_avgs_main_options);
			
			//set up mini version
			var ' .$flotname .'_overview = 	jQuery.plot(jQuery("#' .$flotname .'_overview"), data, {
				series: {
					lines: { show: true, lineWidth: 1 },
					shadowSize: 0
				},
				legend: { show: false},
				xaxis: { tickDecimals: 0 },
				yaxis: { min: 0 },
				grid: { color: "#999", backgroundColor: "#f8f8f8", borderWidth: 0 },
				selection: { mode: "x" }
			});
    }

    plotAccordingToChoices();
	});
</script>
';

	return $result;
}

//spits out a table of comments given an array (with a count field that has zero)
function printComments($array) {
	$result = "
	<h2>Evaluation Comments</h2>
	<table>\n
	<tr><th>Question</th><th>Responses</th></tr>\n
	";
	
	foreach($array as $key=>$a) {
		if($a['count'] == 0) {
			$result .= "<tr><td>$key</td><td>";
			foreach($a['comments'] as $c) { if(strlen($c)) $result .= "$c<br>"; }
			$result .= "</td></tr>\n";
		}
	}
		
	$result .= "</table>";	
	//$result .= print_r($array, true);	
	return $result;
}

//print out all eval questions
function printEvalQs($sections, $questions) {
	$result = "<h2>Evaluation Questions</h2>";
	foreach($sections as $section) {
		$result .= "<h3>$section->title</h3>\n$section->description<br><br>\n<table>\n";
		foreach($questions[$section->id] as $question) {
			$result .= "<tr><td>$question->slug:</td><td>$question->question</td></tr>\n";
		}
		$result .="</table>\n";
	}	
	
	return $result;
}

function printEvals($evals) {
	foreach($evals as $eval) {
	   $elems_ar = explode('&', $eval['answers']);     			// get all variables
	   for ($i = 0; $i < count($elems_ar); $i++) {
		  list($key, $val) = explode('=', $elems_ar[$i]); 	// split variable name from value
		  $key = urldecode($key);
		  $val = urldecode($val);
		  $answers_array[$eval['id']][$key] = $val;     	// store to indexed array
		}
	}
	
	$result = "<h2>Evaluations</h2>";
	
	foreach($evals as $eval) {
		$result .= "<h3><a href='" .$eval['siteurl'] ."'>" .$eval['studentid'] ."</a></h3>\n";
		$result .= "<em>Evaluator:</em> " .$eval['profid'] .", <em>Submitted:</em> " .$eval['submittime'] ."<br>\n";
		
		$result .= "<table>\n";
		foreach($answers_array[$eval['id']] as $key=>$value) {
			$result .= "<tr><td>" .$key ."</td><td>" .$value ."</td></tr>\n";
		}
		$result .="</table><br><br>\n";
	}	
	
	return $result;
}

function interRaterReliability($evals) {
	//process the evals into raters array
	foreach($evals as $eval) {
		$elems_ar = explode('&', $eval['answers']);                   // get all variables
		for ($i = 0; $i < count($elems_ar); $i++) {
			list($key, $val) = explode('=', $elems_ar[$i]); // split variable name from value
		  	$elems[urldecode($key)] = $val = urldecode($val);
		}
		
		$raters[$eval['profid']]['evals'][$eval['studentid']] = $elems;
	}
	
	//prep the raters array
	foreach($raters as $rid=>$r) {
		foreach($r['evals'] as $e) {
			foreach($e as $key=>$value) {
				$raters[$rid]['reliability'][$key]['total_diff'] = 0;
				$raters[$rid]['reliability'][$key]['count'] = 0;
				$raters[$rid]['na_count'] = 0;
			}
		}
	}
	
	//calculate the overlap betweeen the raters
	foreach($raters as $rid=>$rater) { 									//loop through all the raters
		foreach($raters as $cid=>$compare) {							//inside that, loop through raters to compare results
			if($rid != $cid) {											//make sure I'm not comparing with myself
				foreach($rater['evals'] as $r_sid=>$r_eval) {			//loop through the rater's evals
					foreach($compare['evals'] as $c_sid=>$c_eval) {		//loop through the comparer's evals to see if they did the same student
						if($r_sid == $c_sid) {							//if they did the same student, look at actual answers
							foreach($r_eval as $r_key=>$r_value) {		//cycle through each of the individual questions
								if($c_eval[$r_key] && is_numeric($r_value) && $r_value>0 && $c_eval[$r_key]>0 ) {					//if the compare array has this key, and if the value is a number, not a comment, and if both values aren't zero (n/a)
									$raters[$rid]['reliability'][$r_key]['total_diff'] += abs($r_value - $c_eval[$r_key]) /6 *100; 	//add the absolute value of the diff between the answers then convert to percentage !!!hard coded to 6!!!
									$raters[$rid]['reliability'][$r_key]['count'] ++; 	//increment a counter that tracks how many times there's an overlap (will be used to avg the total)
								}
							}
						}
					}
				}
			}
		}
		//populate na_options
		foreach($rater['evals'] as $r_sid=>$r_eval) {
			foreach($r_eval as $r_key=>$r_value) {
				if($r_value!='' && $r_value==0) $raters[$rid]['na_count']++; // !='' makes sure we don't count blank comments
			}
		}
	}
	
	//print the results as a table
	$return = '<h2>Interrater Reliability</h2>';
	foreach($raters as $rid=>$rater) {
		$return .= "<h3>$rid</h3> <table><tr><th>Question</th><th>Average Variation</th></tr>";
		if($rater['na_count'] > 0) { $return .= ($rater['na_count'] >= 3 ?"<tr><td class='alert'>Questions answered N/A</td><td class='alert'>".$rater['na_count']."</td></tr>" :"<tr><td>Questions answered N/A</td><td>".$rater['na_count']."</td></tr>"); }
		foreach($rater['reliability'] as $key=>$a) {
			if($a['count']) { 							//make sure we don't divide by zero
				$avg = round($a['total_diff']/$a['count'], 2);
				$return .= ($avg >= 33 ?"<tr><td class='alert'>$key</td><td class='alert'>$avg</td></tr>" :"<tr><td>$key</td><td>$avg</td></tr>"); //testing for >=2 highlights high values
			}
		}
		$return .= "</table>";
	}
	
	return $return;
}

function printTaxes($evals) {
	$students = array();
	$taxcount = array();
	$return = '';
	foreach($evals as $e) {
		if(!in_array($e['studentid'], $students) ) { 							//test if we have already loaded this student's taxes
			$students[] = $e['studentid'];	
			if(!empty($e['taxonomies'])) {
				foreach($e['taxonomies'] as $tax_key=>$tax_val) {
					foreach($tax_val as $term_key=>$term_val) {
						if (isset($taxcount[$tax_key][$term_key]) ) $taxcount[$tax_key][$term_key] += $term_val; 
						else $taxcount[$tax_key][$term_key] = $term_val;
					}
				}
				/*
				$elems_ar = explode('&', $e['taxonomies']);                   	// get all variables
				for ($j = 0; $j < count($elems_ar); $j++) {
				  list($key, $val) = explode('=', $elems_ar[$j]); 				// split variable name from value
				  if(array_key_exists($key,$taxcount)) $taxcount[$key]+=$val;	// if there's already the key, just add
				  else $taxcount[$key] = $val;									//if the key doesn't exist, create it
				}
				*/
			}
		}
	}
	
	$return .=  "<h2>Taxonomy Report</h2>\n";
	foreach($taxcount as $tax_key=>$tax_val) {
		$return .= "<h3>$tax_key</h3>\n<table>\n<tr><th>Term</th><th>Count</th></tr>\n";
		foreach($tax_val as $term_key=>$term_val) {
			$return .= "<tr><td>$term_key</td><td>$term_val</td></tr>\n";
		}
		$return .= "</table>\n";
	}
	
	return $return;
}

function createENGWHistory($start, $end) {
	if($start['month']>8) $start['year']++;
	if($end['month']>8) $end['year']++;
	
	/* this sets up the history on a scale of 6 - the next one sets it up on a scale of 100
	$history[2012] = array('write_for_situation' => array('avg' => 6*.8910), 'analyze_situation' => array('avg' => 6*0.8743), 'writing_process' => array('avg' => 6*0.8718), 'voice' => array('avg' => 6*0.9038), 'conventions' => array('avg' => 6*0.8763), 'visual_presentation' => array('avg' => 6*0.8583), 'peer_review' => array('avg' => 6*0.8551));	
	$history[2011] = array('write_for_situation' => array('avg' => 6*.8942), 'analyze_situation' => array('avg' => 6*0.8657), 'writing_process' => array('avg' => 6*0.8686), 'voice' => array('avg' => 6*0.9029), 'conventions' => array('avg' => 6*0.8429), 'visual_presentation' => array('avg' => 6*0.8671), 'peer_review' => array('avg' => 6*0.8442));	
	$history[2010] = array('write_for_situation' => array('avg' => 6*.8803), 'analyze_situation' => array('avg' => 6*0.8382), 'writing_process' => array('avg' => 6*0.8526), 'voice' => array('avg' => 6*0.8901), 'conventions' => array('avg' => 6*0.8553), 'visual_presentation' => array('avg' => 6*0.8559), 'peer_review' => array('avg' => 6*0.8382));
	$history[2009] = array('write_for_situation' => array('avg' => 6*.8750), 'analyze_situation' => array('avg' => 6*0.8675), 'writing_process' => array('avg' => 6*0.8717), 'voice' => array('avg' => 6*0.8879), 'conventions' => array('avg' => 6*0.8484), 'visual_presentation' => array('avg' => 6*0.8879), 'peer_review' => array('avg' => 6*0.8589));	
	$history[2008] = array('write_for_situation' => array('avg' => 6*.8654), 'analyze_situation' => array('avg' => 6*0.8298), 'writing_process' => array('avg' => 6*0.8216), 'voice' => array('avg' => 6*0.8462), 'conventions' => array('avg' => 6*0.7882), 'visual_presentation' => array('avg' => 6*0.8548), 'peer_review' => array('avg' => 6*0.8071));
	$history[2007] = array('write_for_situation' => array('avg' => 6*.8490), 'analyze_situation' => array('avg' => 6*0.8400), 'writing_process' => array('avg' => 6*0.8110), 'voice' => array('avg' => 6*0.8380), 'conventions' => array('avg' => 6*0.7930), 'visual_presentation' => array('avg' => 6*0.8410), 'peer_review' => array('avg' => 6*0.8300));	
	*/
	
	$history[2012] = array('write_for_situation' => array('avg' => 100*.8910), 'analyze_situation' => array('avg' => 100*0.8743), 'writing_process' => array('avg' => 100*0.8718), 'voice' => array('avg' => 100*0.9038), 'conventions' => array('avg' => 100*0.8763), 'visual_presentation' => array('avg' => 100*0.8583), 'peer_review' => array('avg' => 100*0.8551));	
	$history[2011] = array('write_for_situation' => array('avg' => 100*.8942), 'analyze_situation' => array('avg' => 100*0.8657), 'writing_process' => array('avg' => 100*0.8686), 'voice' => array('avg' => 100*0.9029), 'conventions' => array('avg' => 100*0.8429), 'visual_presentation' => array('avg' => 100*0.8671), 'peer_review' => array('avg' => 100*0.8442));	
	$history[2010] = array('write_for_situation' => array('avg' => 100*.8803), 'analyze_situation' => array('avg' => 100*0.8382), 'writing_process' => array('avg' => 100*0.8526), 'voice' => array('avg' => 100*0.8901), 'conventions' => array('avg' => 100*0.8553), 'visual_presentation' => array('avg' => 100*0.8559), 'peer_review' => array('avg' => 100*0.8382));
	$history[2009] = array('write_for_situation' => array('avg' => 100*.8750), 'analyze_situation' => array('avg' => 100*0.8675), 'writing_process' => array('avg' => 100*0.8717), 'voice' => array('avg' => 100*0.8879), 'conventions' => array('avg' => 100*0.8484), 'visual_presentation' => array('avg' => 100*0.8879), 'peer_review' => array('avg' => 100*0.8589));	
	$history[2008] = array('write_for_situation' => array('avg' => 100*.8654), 'analyze_situation' => array('avg' => 100*0.8298), 'writing_process' => array('avg' => 100*0.8216), 'voice' => array('avg' => 100*0.8462), 'conventions' => array('avg' => 100*0.7882), 'visual_presentation' => array('avg' => 100*0.8548), 'peer_review' => array('avg' => 100*0.8071));
	$history[2007] = array('write_for_situation' => array('avg' => 100*.8490), 'analyze_situation' => array('avg' => 100*0.8400), 'writing_process' => array('avg' => 100*0.8110), 'voice' => array('avg' => 100*0.8380), 'conventions' => array('avg' => 100*0.7930), 'visual_presentation' => array('avg' => 100*0.8410), 'peer_review' => array('avg' => 100*0.8300));	
	
	for ($y=2012; $y>2006; $y--) {
		if ($end['year']<$y) unset($history[$y]);
		if ($start['year']>$y) unset($history[$y]);
	}
	
	return $history;		
}