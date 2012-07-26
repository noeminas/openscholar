<?php
/**
 * Full Node View
 * Displays the citation in a field that can be clicked to select all of it
 * Also displays abstract, full text and notes if enabled.
 * @param unknown_type $vars
 */
function theme_os_publications_tabular(&$vars) {
  $node = $vars['node'];
  $base = $vars['base'];

  $fields = _biblio_get_field_information($node->biblio_type, TRUE);
  _biblio_localize_fields($fields);

  /* This field hasn't been added yet
  if ($node->biblio_url) {
    $attrib = (variable_get('biblio_links_target_new_window', null)) ? array('target' => '_blank') : array();
    $url_title = db_result(db_query('SELECT biblio_url_title FROM {biblio} WHERE nid = %d', $node->nid));
    $url_title = (isset($url_title) && $url_title) ? $url_title : 'Website';
    $node->biblio_url = l($url_title, $node->biblio_url, $attrib);
  }
  */

  $citation = _os_publications_plain_citation($node);

  // These fields should not be printed under any circumstances
  // They appear elsewhere with special handling, either in the citation
  // or elsewhere in this function
  $hidden = array(
    'biblio_pages',
    'biblio_publisher',
    'biblio_volume',
    'biblio_issue',
    'biblio_year',
    'biblio_place_published',
    'biblio_type_name',
    'biblio_secondary_title',
    'biblio_coins',
    'biblio_lang',
  );
  $rows = array();

  foreach ($fields as $f) {
    if (in_array($f['name'], $hidden)) continue;  // field has already been processed. do nothing
    if (!isset($node->$f['name'])) continue;  // field doesn't exist. do nothing

    $value = $node->$f['name'];
    $display_title = true;

    switch ($f['name']) {
      case 'biblio_url':
        // no need to check plain. l does it for us
        $data = '<p>'.l($node->biblio_url_title, $value).'</p>';
        $display_title = false;
        break;
      case 'biblio_url_title':
        break;
      default:
        if ($f['type'] == 'text_format') {
          $data = check_markup($value, $node->biblio_formats[$f['name']]);
        }
        else {
          $data = check_plain($value);
        }
    }
    if (trim($data)) {
      if ($display_title) {
        $rows[] = array(
          array(
            'data' => t($f['title']),
            'class' => 'biblio-row-title biblio-field-title-'.str_replace('_', '-', str_replace('biblio_', '', $f['name']))
          ),
          array(
            'data' => $data,
            'class' => 'biblio-field-contents-'.str_replace('_', '-', str_replace('biblio_', '', $f['name']))
          )
        );
      }
      else {
        $rows[] = array(
          1 => array(
            'data' => $data,
            'class' => 'biblio-field-contents-'.str_replace('_', '-', str_replace('biblio_', '', $f['name']))
          )
        );
      }
    }
  }
  $output = '<div id="biblio-node">';
  $output .= filter_xss($node->biblio_coins, array('span'));
  $output .= '<h3>Citation:</h3><div id="citation-wrapper">'.$citation.''.theme('biblio_export_links', array('node' => $node)).'</div>';

  foreach ($rows as $row){
    if (isset($row[0]['data']) && $row[0]['data'])
      $output .= "<h3>".$row[0]['data'].":</h3>";
    $output .= $row[1]['data'];
  }

  if (isset($node->body[LANGUAGE_NONE][0]['value']) && strlen(trim($node->body[LANGUAGE_NONE][0]['value'])) && user_access('view full text')) {
    $output .= '<h3>Full Text</h3>';
    $output .= check_markup($node->body[LANGUAGE_NONE][0]['value'], $node->body[LANGUAGE_NONE][0]['format'], FALSE);
  }

  $output .= '</div>';

  return $output;
}

/**
 * Get the citation for a biblio node as just plain text
 */
function _os_publications_plain_citation($node) {
  $style = biblio_get_style();
  $base  = variable_get('biblio_base', 'biblio');

  // Note that this is a working paper after the title and before the year
  if ($style == 'chicago' && in_array($node->biblio_type, array(1000)) && empty($node->biblio_publisher)){
    $node->biblio_publisher = "Working Paper";
  }

  $citation = theme('biblio_style', array('node' => $node, 'base' => $base, 'style' => $style));

  // add url to citation
  if (function_exists('shorten_url') && variable_get('os_publications_shorten_citations', 0)) {
    global $base_url;
    /* replace punctuation at end of citation with period.
     * biblio_coins follow end of citation and block an easy RE match, so we're appending them.
     */
    $coins = filter_xss($node->biblio_coins, array('span'));
    $re = '/' .  '[!-~] (<\/\w*>)?' . preg_quote($coins, '/') . '/';

    if (preg_match( $re, $citation)) {
      $citation = preg_replace($re, ", </span>$coins", $citation);
    }

    $citation .= ' copy at ' . shorten_url(url('node/'.$node->nid, array('absolute'=>TRUE)));
  }

  $citation = strip_tags($citation);
  drupal_add_js('function selectText(ele) {
	try {
  	var div = document.createRange();
    div.setStartBefore(ele);
    div.setEndAfter(ele) ;
  	window.getSelection().addRange(div);
  }
  catch (e) {
  	// we\'re in internet explorer
  	div = document.selection.createRange();
  	div.moveToElementText(ele);
  	div.select();
  }
}','inline');
  $citation = "<div class=\"biblio-citation\" onclick=\"if (event.target) selectText(event.target); else selectText(this);\">{$citation}</div>";

  return $citation;
}

function theme_os_publications_export_links($variables) {

  global $pager_total_items;
  $node = $variables['node'];
  $filter = ($variables['node'] == NULL && isset($variables['filter'])) ? $variables['filter'] : array();
  $links = array();
  $output = '';

  if (biblio_access('export')) {
    $show_link = variable_get('biblio_lookup_links', array('google' => TRUE));
    $lookup_links = module_invoke_all('biblio_lookup_link', $node);
    if ($show_link['google'] && !empty($node)) {
      $lookup_links['biblio_google_scholar'] = theme('google_scholar_link', array('node' => $node));
    }
    $nid = (isset($node->nid)) ? $node->nid : NULL;
    $export_links = module_invoke_all('biblio_export_link', $nid, $filter);
    $links = array_merge($lookup_links, $export_links);
  }
  $title = 'Export';
  if (empty($node) && !empty($links)) {
    $title = t('Export @count results', array('@count' => $pager_total_items[0])) . ': ';
  }

  return theme('ctools_dropdown', array('links' => $links, 'title' => $title, 'attributes' => array('class' => array('biblio-export-buttons'))));
}


/**
 * @param $form
 * @return unknown_type
 */
function theme_os_publications_contributors($variables) {
  $form = $variables['form'];
  $rows = array();
  $display_role = true;
  $headers = array('', t('Name'), t('Role'), t('Weight'));
  drupal_add_tabledrag($form['#id'], 'order', 'sibling', 'rank');

  foreach (element_children($form) as $key) {
    // No need to print the field title every time.
//    unset($form[$key]['name']['#title'], $form[$key]['auth_type']['#title'], $form[$key]['auth_category']['#title']);
    // Add class to group weight fields for drag and drop.
    $form[$key]['rank']['#attributes']['class'] = array('rank');

    // Build the table row.
    $row = array('');
    $row[] = array('data' => drupal_render($form[$key]['name']).drupal_render($form[$key]['auth_category']),
                   'class' => array('biblio-contributor'));
    $row[] = array('data' => drupal_render($form[$key]['auth_type']),
                     'class' => array('biblio-contributor-type'));
    $row[] = drupal_render($form[$key]['rank']);
    $rows[] = array('data' => $row, 'class' => array('draggable'));
  }
  $output = theme('table', array('header' => $headers, 'rows' => $rows, 'attributes' => array('id' => $form['#id'])));
  //$output .= drupal_render_children($form);
  return $output;
}