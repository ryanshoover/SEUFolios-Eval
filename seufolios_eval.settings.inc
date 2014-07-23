<?php 
require_once('seufolios_eval.module');

// *****
// generates the settings page
function seufolios_settings_form($form, &$form_state) {
	
	//get wp_url
	$go_for_depts = false;
	$result = db_query("SELECT settings FROM {seufolios_eval_settings} WHERE name = :name", array(':name'=>'wp_url') );
	$data = $result->fetchObject();
	if($data) {
		$wp_url = unserialize($data->settings);
		$go_for_depts = true;
	}
	else $wp_url = 'http://seufolios.org';
	
	//get wp_url_key
	$result = db_query("SELECT settings FROM {seufolios_eval_settings} WHERE name = :name", array(':name'=>'wp_url_key') );
	$data = $result->fetchObject();
	if($data) $wp_url_key = unserialize($data->settings);
	else $wp_url_key = 'abcdefg1234567';
	
	//get wp_dept_data
	if($go_for_depts) {
		$dept_data = get_wp_dept_data();

		if(!$dept_data) {
			$go_for_depts = false;
		} else {
			$depts[] = '--choose a department--';
			foreach($dept_data as $dept) 
				$depts[$dept->abbr] = $dept->name;
		}
		
	}
	
  $form['#tree'] = TRUE;
	$form['explanation'] = array(
	  '#type' => 'item',
	  '#markup' => '<p>Use this form to set the department-specific features.</p>'
	);
	
	$form['url_info'] = array(
	  '#type' => 'fieldset',
	  '#title' => t('Information about the SEUFolios WordPress site'),
	);
	
	$form['url_info']['wp_url'] = array(
	  '#type' => 'textfield',
	  '#title' => t('URL'),
	  '#description' => t('URL of the SEUFolios installation'),
	  '#default_value' => $wp_url,
	  '#size' => 40,
	  '#maxlength' => 120,
	  '#required' => TRUE,
	);
	
	$form['url_info']['wp_url_key'] = array(
	  '#type' => 'textfield',
	  '#title' => t('Key'),
	  '#description' => t('Access key for the SEUFolios installation'),
	  '#default_value' => $wp_url_key,
	  '#size' => 40,
	  '#maxlength' => 120,
	  '#required' => TRUE,
	);
	
	if($go_for_depts) {
		
		$form['dept_select'] = array(
       '#type' => 'select',
       '#title' => t('Choose a department'),
       '#options' => $depts,
       '#description' => t('Choose the department that you want to modify settings for.'),
			 '#ajax' => array(
			       'callback' => 'seufolios_settings_form_callback',
			       'wrapper' => 'dept_options',
			       'effect' => 'fade',
			     ),
		 );
		 
		$form['dept_options'] = array (
			'#prefix' => '<div id="dept_options">',
		  '#suffix' => '</div>',
			'#type' => 'fieldset',
			'#title' => 'Department settings',
	 	);
		
		/*
 		$form['var_dump'] = array(
 			'#type' => 'item',
			'#title' => 'Dump of WP return',
 			'#markup' => "<pre>".print_r($dept_data, true)."</pre>",
 		);
		*/
		
	  // Since checkboxes return TRUE or FALSE, we have to check that
	  // $form_state has been filled as well as what it contains.
	  if (!empty($form_state['values']['dept_select']) && $form_state['values']['dept_select']) {
			//pick the dept out of the whole set
			foreach($dept_data as $d) {
				if($d->abbr == $form_state['values']['dept_select']) $dept = $d;
			}
			//clear the count of slos
			//unset($form_state['num_names']);
			
			//SLOs for the dept
			//borrowed from example module
			$result = db_query("SELECT settings FROM {seufolios_eval_settings} WHERE name = :name AND dept = :dept", array(':name'=>'slos', ':dept'=>$form_state['values']['dept_select']) );
			$data = $result->fetchObject();
			if($data) {
				$slos = unserialize($data->settings);
				if (empty($form_state['num_names'])) $form_state['num_names'] = count($slos);
			} 
			
		  $form['dept_options']['slos_fieldset'] = array(
		    '#type' => 'fieldset',
		    '#title' => t('Student Learning Outcomes'),
				'#description' => t('These are the categories you want to base report calculations on. The report will run averages for each of these. You\'ll define how they map to the WordPress evaluation questions below. The categories are normally student learning outcomes.'),
		    '#prefix' => '<div id="slos-fieldset-wrapper">',
		    '#suffix' => '</div>',
		  );

		  if (empty($form_state['num_names'])) {
		    $form_state['num_names'] = 1;
		  }
		  for ($i = 0; $i < $form_state['num_names']; $i++) {
			  $form['dept_options']['slos_fieldset']['slo'][$i]['title'] = array(
		      '#type' => 'textfield',
		      '#title' => t('SLO #' .($i+1)),
					'#default_value' => $slos[$i]['title'],
		    );
			  $form['dept_options']['slos_fieldset']['slo'][$i]['description'] = array(
		      '#type' => 'textfield',
		      '#title' => t('Description #' .($i+1)),
					'#default_value' => $slos[$i]['description'],
		    );
		  }
		  $form['dept_options']['slos_fieldset']['add_slo'] = array(
		    '#type' => 'submit',
		    '#value' => t('Add one more'),
		    '#submit' => array('seufolios_settings_form_slo_add_one'),
		    '#ajax' => array(
		      'callback' => 'seufolios_settings_form_slo_callback',
		      'wrapper' => 'slos-fieldset-wrapper',
		    ),
		  );
		  if ($form_state['num_names'] > 1) {
		    $form['dept_options']['slos_fieldset']['remove_slo'] = array(
		      '#type' => 'submit',
		      '#value' => t('Remove one'),
		      '#submit' => array('seufolios_settings_form_slo_remove_one'),
		      '#ajax' => array(
		        'callback' => 'seufolios_settings_form_slo_callback',
		        'wrapper' => 'slos-fieldset-wrapper',
		      ),
		    );
		  }
			
			//create the form entries
			if(isset($slos)) {
				$result = db_query("SELECT settings FROM {seufolios_eval_settings} WHERE name = :name AND dept = :dept", array(':name'=>'slo-ques-relationship', ':dept'=>$form_state['values']['dept_select']) );
				$data = $result->fetchObject();
				$rels = unserialize($data->settings);
				
				$header[] = 'Questions';
				foreach($slos as $s) 
					$header[] = $s['title'];
				
				$form['dept_options']['table_heading'] = array(
					'#type' => 'item',
					'#title' => 'SLO - Question Relationships',
					'#markup' => '<p>Enter the percent values (as whole numbers) that should be used to calculate the student learning outcomes. The questions are from the WordPress site and the SLOs are from the settings above. You may have a 1-1 correlation if your questions are the same as your SLOs.</p><p>Please make sure that each column totals 100.</p>',
				);
				
				$form['dept_options']['ques_table'] = array(
				  '#prefix' => '<div id="ques_table_wrapper">',
				  '#suffix' => '</div>',
				  '#tree' => TRUE,
				  '#theme' => 'table',
				  '#header' => $header,
				  '#rows' => array(),
				);
				
				foreach($dept->eval_sections as $sec) {
					//set section headings
					$col1 = array(
						'#id' => 'col-' .$sec->title,
						'#type' => 'item',
						'#markup' => '<span class="ques-table-sec">' .$sec->title .'</span>',
					);
					$fields[] = array('header-'.$sec->title => &$col1);
					$row[] = array('data' => &$col1, 'colspan' => (count($slos)+1) );
					
					$form['dept_options']['ques_table'][] = $fields;
					$form['dept_options']['ques_table']['#rows'][] = $row;
					unset($col1, $fields, $row);
					
					//add questions below section heading
					for ($i = 0; $i < count($sec->questions); $i++) {
						$fields = array();
						$row = array();
						$title = $sec->questions[$i]->slug;
						
						$col1 = array(
							'#id' => 'col-' .$title,
							'#type' => 'item',
							'#markup' => '<span class="ques-table-ques">' .$title .'</span>',
						);
						$fields[] = array('val-'.$sec->title.'-'.$title => &$col1);
						$row[]= array('data' => &$col1);
						
						//create textfields in each cell
						for ($j=0; $j < count($slos); $j++) {
							$cell = array(
								'#id' => 'val-'.$sec->title.'-'.$title,
								'#type' => 'textfield',
								'#default_value' => $rels['val-'.$j.'-'.$title],
								'#size'=> 3,
							);
							$fields[] = array('val-'.$j.'-'.$title => &$cell);
							$row[] = array('data' => &$cell);
							unset($cell);
						}
							
						$form['dept_options']['ques_table'][] = $fields;
						$form['dept_options']['ques_table']['#rows'][] = $row;
						unset($col1, $fields, $row);
					}
				}	
				
				/*
		 		$form['dept_options']['var_dump'] = array(
		 			'#type' => 'item',
					'#title' => 'Dump of WP return',
		 			'#markup' => "<pre>".print_r($dept_data, true)."</pre>",
		 		);
				*/
			}
	  }
	}
	
	// Submit button
	$form['submit'] = array(
	'#type' => 'submit',
	'#value' => t('Save settings'),
	);
	
	return $form;
}

// *****
// helper functions
function seufolios_settings_form_callback($form, $form_state) {
	return $form['dept_options'];
}

function seufolios_settings_form_slo_callback($form, $form_state) {
  return $form['dept_options']['slos_fieldset'];
}


function seufolios_settings_form_slo_add_one($form, &$form_state) {
  $form_state['num_names']++;
  $form_state['rebuild'] = TRUE;
}

function seufolios_settings_form_slo_remove_one($form, &$form_state) {
  if ($form_state['num_names'] > 1) {
    $form_state['num_names']--;
  }
  $form_state['rebuild'] = TRUE;
}

// *****
// validate the form
function seufolios_settings_form_validate($form, &$form_state) {
	//clear any existing errors
	drupal_get_messages('error');
	/*
	//check url settings
	$data = get_wp_dept_data();

	if(!$data) {
		form_set_error('URL', 'Oops. Looks like we couldn\'t connect to your WordPress site. Please check the site url and key. WordPress responded with: ' .$wp_data);
	}
	*/
	//update individual department question-slo relationship
	if(isset($form_state['values']['dept_options']['ques_table'])) {
		$temp = $form_state['values']['dept_options']['ques_table'];
		//validation
		foreach($temp as $t1) {
			foreach($t1 as $slo=>$t2) {
				foreach($t2 as $key=>$val)
					$validate[$slo][$key] = $val;
			}
		}
		foreach($validate as $slo_key=>$slo) {
			$totals[$slo_key] = 0;
			foreach($slo as $key=>$val) {
				$totals[$slo_key] += $val; 
			}
		}
		
		foreach($totals as $key=>$val) {
			if($val != 100 && $val != 0) {
				form_set_error('SLO - Question Relationships', t('Double check your values in the SLO-Question relationship table. At least one of your columns doesn\'t add up to 100.') );
			}
		}
	}
}

// *****
// submit the form
function seufolios_settings_form_submit($form, &$form_state) {
	$wp_url = serialize( $form_state['values']['url_info']['wp_url'] );
	$wp_url_key = serialize( $form_state['values']['url_info']['wp_url_key'] );

	//update wp_url
	$result = db_query("SELECT settings FROM {seufolios_eval_settings} WHERE name = :name", array(':name'=>'wp_url') );
	$data = $result->fetchObject();

	if($data) {
		$vid = db_update('seufolios_eval_settings')
			->fields(array('settings' => $wp_url))
			->condition('name', 'wp_url', '=')
			->execute();		
	} else {
		$vid = db_insert('seufolios_eval_settings')
			->fields(array(
				'name' => 'wp_url',
				'settings' => $wp_url,
			))
			->execute();
	}
	
	//update wp_url_key
	$result = db_query("SELECT settings FROM {seufolios_eval_settings} WHERE name = :name", array(':name'=>'wp_url_key') );
	$data = $result->fetchObject();

	if($data) {
		$vid = db_update('seufolios_eval_settings')
			->fields(array('settings' => $wp_url_key))
			->condition('name', 'wp_url_key', '=')
			->execute();
	} else {
		$vid = db_insert('seufolios_eval_settings')
			->fields(array(
				'name' => 'wp_url_key',
				'settings' => $wp_url_key,
			))
			->execute();
	}
	
	if(isset($form_state['values']['dept_select']))
		$dept_select = $form_state['values']['dept_select'];
	
	//update individual department slos
	if(isset($form_state['values']['dept_options']['slos_fieldset']['slo'])) {
		$result = db_query("SELECT settings FROM {seufolios_eval_settings} WHERE name = :name AND dept = :dept", array(':name'=>'slos', ':dept'=>$dept_select) );
		$data = $result->fetchObject();
		$slos = serialize($form_state['values']['dept_options']['slos_fieldset']['slo']);
		if($data) {
			$vid = db_update('seufolios_eval_settings')
				->fields(array('settings' => $slos))
				->condition('name', 'slos', '=')
				->execute();
		} else {
			$vid = db_insert('seufolios_eval_settings')
				->fields(array(
					'name' => 'slos',
					'dept' => $dept_select,
					'settings' => $slos,
				))
				->execute();
		}
	}
	
	//update individual department question-slo relationship
	if(isset($form_state['values']['dept_options']['ques_table'])) {
		$result = db_query("SELECT settings FROM {seufolios_eval_settings} WHERE name = :name AND dept = :dept", array(':name'=>'slo-ques-relationship', ':dept'=>$dept_select) );
		$data = $result->fetchObject();
		
		$temp = $form_state['values']['dept_options']['ques_table'];
		//process into a single level array for insert into db
		foreach($temp as $t1) {
			foreach($t1 as $t2) {
				foreach($t2 as $key=>$val)
					$insert_arr[$key] = $val;
			}
		}
		
		$insert = serialize($insert_arr);
		if($data) {
			$vid = db_update('seufolios_eval_settings')
				->fields(array('settings' => $insert))
				->condition('name', 'slo-ques-relationship', '=')
				->execute();
		} else {
			$vid = db_insert('seufolios_eval_settings')
				->fields(array(
					'name' => 'slo-ques-relationship',
					'dept' => $dept_select,
					'settings' => $insert,
				))
				->execute();
		}
	}
	
}