<?php
// $Id$

/**
 * @file panels_views.module
 *
 * Provides views as panels content, configurable by the administrator.
 * Each view provided as panel content must be configured in advance,
 * but once configured, building panels with views is a little bit simpler.
 */

function views_content_admin_page() {
  $form['ctools_content_all_views'] = array(
    '#type' => 'checkbox',
    '#title' => t('Make all views available as panes'),
    '#description' => t("If checked, all views will be made available as content panes to be added to content types. If not checked, only Views that have a 'Content pane' display will be available as content panes."),
    '#default_value' => variable_get('ctools_content_all_views', TRUE),
  );

  return system_settings_form($form);

}