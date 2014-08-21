<?php 
/**
 * Listing paths that use the meta tags.
 */
function mymetatags_list() {  
  $query = 'SELECT * FROM {mymetatags}';
  $result= db_query($query);
  $returnedHTML = '<ul>';
  foreach ($result as $record){
    $returnedHTML .= '<li><b>' .t('Path: '). $record->normal_path . ' </b><a href="/admin/config/search/mymetatags/' . $record->mtid . '/delete">' . t('Delete') . '</a></li>';
    $unserializedData = unserialize($record->data);
    $returnedHTML .= '<ul>';
    foreach ($unserializedData as $key=>$value){
      $returnedHTML .= '<li>' . $key .'</li>';
    }
    $returnedHTML .= '</ul>';
  }
  $returnedHTML .= '</ul>';
  return $returnedHTML;
}

/**
 * Confirm mapped meta tags deletion
 */
function mymetatags_map_delete_confirm($form, $form_state, $mapping) {
  $form['_delete_mapping'] = array(
    '#type'   => 'value',
    '#value'  => $mapping,      
  );
  return confirm_form(
      $form, 
      "Are you sure you'll want to delete this meta tag mapping?", 
      isset($_GET['destination']) ? $_GET['destination'] : "admin/config/search/mymetatags"
  );
}

/**
 * Delete mapped meta tags
 */
function mymetatags_map_delete_confirm_submit($form, &$form_state) {
  $values = $form_state['values'];
  
  if ($form_state['values']['confirm']) {
    $mapping = $form_state['values']['_delete_mapping'];  
    db_delete('{mymetatags}')->condition('mtid',$mapping->mtid)->execute();    
    drupal_set_message(t('Meta Tag Mapping has been deleted successfully.'));
  }
  
  drupal_goto("admin/config/search/mymetatags");
}

/**
 * Manage meta tags
*/
function mymetatags_configure(){
  $default = "keywords|" . t("description of keywords") . "\n" . "definition|" . t("description of definition");
  $default = variable_get('mymetatags', $default);
  $form['mymetatags'] = array(
      '#type'           => 'textarea',
      '#title'          => t('My Meta Tags'),
      '#default_value'  => $default,
  );  
  $form['#submit'] = array('mymetatags_configure_submit');
  
  return system_settings_form($form);
}

/**
 * Validate meta tags configured
*/
function mymetatags_configure_validate($form, $form_state){
  $values = trim($form_state['values']['mymetatags']);
  
  if($values == ""){
    $values = array();
  }else{
    $values = explode("\r\n",$values);
    $pattern = "/^[a-zA-Z0-9 -]*\|[a-zA-Z0-9 -]*$/";
   
    foreach ($values as $value){
      if(preg_match($pattern, $value)===0){
        form_set_error('mymetatags', t('Please enter format key|description and a line break to separate meta tags.'));
        return;
      }
    }
  }
 
}

/**
 * Submit meta tags configured
 */
 function mymetatags_configure_submit($form, &$form_state){
   $values = explode("\r\n",trim($form_state['values']['mymetatags']));
   $form_state['values']['mymetatags'] = "";
   $values_filtered = array();
   foreach ($values as $value){
    $value = explode('|',$value);
    $values_filtered[$value[0]] = $value[1];
   }
   $values = $values_filtered;
   $must_have = array(
    'keywords'  =>'description of keywords',
    'definition'=> 'description of definition'
   );
   foreach ($must_have as $key => $value){
     if(!array_key_exists($key, $values)){
       $values[$key] = $value;
     }
   }
   $inputed = array();
   foreach ($values as $key => $value) {
     $inputed[] = $key;
     $form_state['values']['mymetatags'] .= $key . '|' . $value . "\n";
   }
   
   $form_state['values']['mymetatags'] = trim($form_state['values']['mymetatags']);
    
   $stored = explode("\n", variable_get('mymetatags'));
   foreach ($stored as &$value) {
      $value = explode('|',$value);
      $value = $value[0];
   }
   
   $to_delete = array_diff($stored, $inputed);
   
   mymetatags_remove_unused($to_delete);
   
 }
 
 /**
  * clean metatags that are not used anymore
  * @arg array $to_delete deleted metatags
  */
 function mymetatags_remove_unused($to_delete=array()) {
   if(count($to_delete)==0){
     return;
   }
   $query = 'SELECT * FROM {mymetatags}';
   $result= db_query($query);
 
   foreach ($result as $record){
     $tags = unserialize($record->data);
     $tags_current_length = count($tags);
     foreach ($tags as $key => &$value){
        foreach($to_delete as $delete){
          if($key == $delete){
            unset($tags[$key]);
          }
        }
     }
     if(count($tags) == 0){
       db_delete('{mymetatags}')->condition('mtid',$record->mtid)->execute();
     }elseif($tags_current_length!=count($tags)){
       $fields = array(
          'data' =>  serialize($tags),
       );
       db_update('{mymetatags}')->fields($fields)->condition('mtid',$record->mtid)->execute(); 
     }
   }
 }