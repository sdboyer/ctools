<?php
// $Id $

/**
 * Specialized implementation of hook_delegator_tasks(). See api-tasks.html for
 * more information.
 */
 function delegator_node_view_delegator_tasks() {
   return array(
     'node_view' => array(
        'title' => t('Node view'),
        'description' => t('The node view task allows you to control which functions will handle the actual job of rendering a node view. The first task that matches the node will be used to display the node. If there are no task handlers listed, or no task handlers are configured to handle a given node, the default Drupal node view mechanism will be used.'),
        'admin title' => 'Node view', // translated by menu system
        'admin description' => 'Overrides for the built in node view handler which allows customized node output.',
        'hook menu' => 'delegator_node_view_menu',
        'hook menu alter' => 'delegator_node_view_menu_alter',
     ),
   );
 }
