<?php
function courses_admin() {
    $output = '';
    
    $header = array(t('Code'), t('Name'), t('Level'), t('Offered in'), t('Operations'));
    $rows = array();
   
    $res = db_query('SELECT code FROM {courses_meta} ORDER BY level, name');
    while ($course_ = db_fetch_object($res)) {
        $course = course_meta_load($course_->code);
        
        $instances_str = '--';
        if ($course->instances) {
            $instances_str = '';
            
            foreach ($course->instances as $instance) {
                $instances_str .= l($instance['term'], 'math/courses/' . $instance['cid']) . ', ';
            }
        }
        
        $rows[] = array($course->code,
                        check_plain($course->name),
                        courses_levels($course->level),
                        $instances_str,
                        l(t('edit'), 'math/courses/courses/' . $course->code . '/edit') . ' ' .
                        l(t('delete'), 'math/courses/courses/' . $course->code . '/delete'));
    }
    
    if (count($rows) == 0) $rows[] = array(array('data' => t('No entries found.'), 'colspan' => 5));
    
    $output .= theme('table', $header, $rows);
    $output .= drupal_get_form('courses_meta_form', NULL);
    
    return $output;
}

function courses_meta_form($form_id, $course_meta = NULL) {
    $form = array();
    
    if ($course_meta)
        $form['#course_meta'] = $course_meta;
    
    $form['course'] = array(
        '#type' => 'fieldset',
        '#title' => $course_meta ? '' : t('Add Course'),
        '#collapsible' => TRUE,
        '#collapsed' => $course_meta ? FALSE : TRUE
    );
    $form['course']['code'] = array('#type' => 'textfield',
                                    '#title' => t('Code'),
                                    '#maxlength' => 100,
                                    '#size' => 10,
                                    '#required' => TRUE,
                                    '#default_value' => $course_meta ? $course_meta->code : '');
    $form['course']['name'] = array('#type' => 'textfield',
                                    '#title' => t('Name'),
                                    '#maxlength' => 255,
                                    '#size' => 30,
                                    '#required' => TRUE,
                                    '#default_value' => $course_meta ? $course_meta->name : '');
    $form['course']['level'] = array('#type' => 'select',
                                     '#title' => t('Level'),
                                     '#options' => courses_levels(),
                                     '#required' => TRUE,
                                     '#default_value' => $course_meta ? $course_meta->level : '');
    
    $form['course']['submit'] = array('#type' => 'submit', '#value' => t('Save'));

    return $form;
}

function courses_meta_form_submit($form, &$form_state) {
    $course_meta = isset($form['#course_meta']) ? $form['#course_meta'] : NULL;
    $values = $form_state['values'];
    
    if ($course_meta) {
        drupal_set_message(t('The course "@course" has been updated.', array('@course' => $values['name'])));
        
        db_query('UPDATE {courses_meta} SET code=\'%s\', name=\'%s\', level=\'%s\' WHERE code=\'%s\'',
                 $values['code'],
                 $values['name'],
                 $values['level'],
                 $course_meta->code);        
    } else {
        drupal_set_message(t('The course "@course" has been added.', array('@course' => $values['name'])));

        db_query('INSERT INTO {courses_meta} (code, name, level) VALUES (\'%s\', \'%s\', \'%s\')',
                 $values['code'],
                 $values['name'],
                 $values['level']);
    }
    
    $form_state['redirect'] = 'math/courses/courses';

}

function courses_meta_delete_form($form_id, $course_meta) {
    $form = array();
    $form['#course_meta'] = $course_meta;
    
    return confirm_form(
        $form,
        t('Are you sure you want to delete the course "@course"?', array('@course' => $course_meta->name)),
        'math/courses/courses',
        t('This will also delete all information related to this course. This action cannot be undone.'),
        t('Delete'),
        t('Cancel'));
}

function courses_meta_delete_form_submit($form, &$form_state) {
    drupal_set_message(t('The course has been deleted.'));
    
    db_query('DELETE FROM {courses_meta} WHERE code=\'%s\'', $form['#course_meta']->code);
    db_query('DELETE FROM {courses} WHERE code=\'%s\'', $form['#course_meta']->code);
    
    menu_rebuild();
    
    $form_state['redirect'] = 'math/courses/courses';    
}

function courses_admin_terms() {
    $output = '';
    
    $header = array(t('Term'), t('Offered Courses'), t('Operations'));
    $rows = array();
    
    $current = courses_current_term();
    
    $result = db_query('SELECT term FROM {courses_terms} ORDER BY term ASC');
    while ($term = db_fetch_object($result)) {
        $offered_courses_str = '--';
        
        $result2 = db_query('SELECT COUNT(*) FROM {courses} WHERE term=\'%s\'', $term->term);
        if (db_result($result2) > 0) {
            $offered_courses_str = '';
            
            $result2 = db_query('SELECT ci.cid, ci.code, cm.name FROM {courses} ci INNER JOIN {courses_meta} cm ON cm.code=ci.code WHERE ci.term=\'%s\'', $term->term);
            while ($course_instance = db_fetch_object($result2)) {
                $offered_courses_str .= l($course_instance->code . ' - ' . check_plain($course_instance->name),
                                          'math/courses/'.  $course_instance->cid) . ', '; 
            }
        }
        
        
        $rows[] = array(
            l($current == $term->term ? $term->term . ' [' . t('current') . ']' : $term->term, 'math/courses/terms/' . $term->term),
                        $offered_courses_str,
                        l(t('edit'), 'math/courses/terms/' . $term->term . '/edit') . ' ' .
                        l(t('delete'), 'math/courses/terms/' . $term->term . '/delete')
                        );
    }
    if (count($rows) == 0) $rows[] = array(array('data' => t('No entries found.'), 'colspan' => 3));
    
    $output .= theme('table', $header, $rows);
    
    $output .= drupal_get_form('courses_term_form', NULL);
    
    return $output;
}

function courses_admin_term($term) {
    $output = '';
    
    if ($term->term == courses_current_term())
        $output .= '<p>** ' . t('This is the current term.') . '**</p>';    
    
    $output .= '<h3>' . t('Offered Courses') . '</h3>';
    
    $header = array(t('Code'), t('Name'), t('Operations'));
    $rows = array();
    $res = db_query('SELECT ci.cid, ci.code, cm.name FROM {courses} ci INNER JOIN {courses_meta} cm ON ci.code=cm.code WHERE ci.term=\'%s\'',
                    $term->term);

    while ($course = db_fetch_object($res)) {
        $rows[] = array(l($course->code, 'math/courses/' . $course->cid),
                        l(check_plain($course->name), 'math/courses/' . $course->cid),
                        l(t('view'), 'math/courses/' . $course->cid) . ' ' .
                        l(t('edit'), 'math/courses/' . $course->cid . '/edit') . ' ' .
                        l(t('delete offering'), 'math/courses/' . $course->cid . '/delete')
                        );
    }
    if (count($rows) == 0) $rows[] = array(array('data' => t('No entries found.'), 'colspan' => 3));    
    
    $output .= theme('table', $header, $rows);
    $output .= drupal_get_form('courses_form', NULL, $term);
    
    return $output;
}

function courses_term_form($form_id, $term = NULL) {
    $form = array();
    
    if ($term)
        $form['#term'] = $term;
    
    $form['term'] = array('#type' => 'fieldset',
                          '#title' => $term ? '' : t('Add Term'),
                          '#collapsible' => TRUE,
                          '#collapsed' => $term ? FALSE : TRUE);
    $form['term']['term'] = array('#type' => 'textfield',
                                  '#title' => t('Term Name'),
                                  '#maxlength' => 100,
                                  '#size' => 20,
                                  '#required' => TRUE,
                                  '#default_value' => $term ? $term->term : '');
    $form['term']['submit'] = array('#type' => 'submit', '#value' => t('Save Term'));
    
    return $form;
}

function courses_term_form_validate($form, &$form_state) {
    $term_name = trim($form_state['values']['term']); 
    $form_state['values']['term'] = $term_name;
}

function courses_term_form_submit($form, &$form_state) {
    $term = isset($form['#term']) ? $form['#term'] : NULL;
    $term_name = $form_state['values']['term'];
    
    
    if ($term) {
        db_query('UPDATE {courses_terms} SET term=\'%s\' WHERE term=\'%s\'', $term_name, $term->term);
        drupal_set_message(t('The term "@term" has been updated.', array('@term' => $term_name)));
    } else {
        drupal_set_message(t('The term "@term" has been added.', array('@term' => $term_name)));
        
        $record = array('term' => $term_name);
        drupal_write_record('courses_terms', $record);
    }
    
    $form_state['redirect'] = 'math/courses/terms/' . $term_name;
    
    menu_rebuild();
}

function courses_term_delete_form($form_id, $term) {
    $form = array();
    $form['#term'] = $term;
    
    return confirm_form(
        $form,
        t('Are you sure you want to delete the term "@term"?', array('@term' => $term->term)),
        'math/courses/terms/' . $term->term,
        t('This will also delete all course information related to this term. This action cannot be undone.'),
        t('Delete'),
        t('Cancel'));
}

function courses_term_delete_form_submit($form, &$form_state) {
    drupal_set_message(t('The term has been deleted.'));
    
    db_query('DELETE FROM {courses_terms} WHERE term=\'%s\'', $form['#term']->term);
    db_query('DELETE FROM {courses} WHERE term=\'%s\'', $form['#term']->term);
    
    menu_rebuild();
    
    $form_state['redirect'] = 'math/courses/terms';    
}

function courses_admin_course($course) {
    $output = '';
    
    $output .= '<h2>' . t('Basic Info.') . '</h2>';
    $output .= '<dl>';
    $output .= '<dt>' . t('Code') . '</dt> <dd>' . check_plain($course->code) . '</dd>';
    $output .= '<dt>' . t('Name') . '</dt> <dd>' . check_plain($course->name) . '</dd>';
    
    $output .= '<dt>' . t('Teachers') . '</dt>';
    $output .= '<dd>';
    if ($course->staff['teachers']) {
        foreach ($course->staff['teachers'] as $teacher)
            $output .= $teacher['name'] . '<br />';
    } else {
        $output .= '--';
    }
    $output .= '</dd>';
    
    $output .= '<dt>' . t('Teaching Assistants') . '</dt>';
    $output .= '<dd>';
    if ($course->staff['teaching assistants']) {
        foreach ($course->staff['teaching assistants'] as $ta)
            $output .= $ta['name'] . '<br />';
    } else {
        $output .= '--';
    }
    $output .= '</dd>';
    
    $output .= '</dl>';
    
    return $output;
}

function courses_form($form_id, $course = NULL, $term = NULL) {
    $form = array();
    
    if ($course) $form['#course'] = $course;
    $form['#term'] = $term;
    
    $form['course'] = array(
        '#type' => 'fieldset',
        '#title' => $course ? '' : t('Add Course Offering'),
        '#collapsible' => TRUE,
        '#collapsed' => $course ? FALSE : TRUE
    );
    
    if (!$course) {
        $courses = array();
        $result = db_query('SELECT cm.code, cm.level, cm.name FROM {courses_meta} cm WHERE NOT EXISTS(SELECT 1 FROM {courses} WHERE code=cm.code AND term=\'%s\') ORDER BY name', $term->term);
        while ($course_meta = db_fetch_object($result)) {
            $courses[courses_levels($course_meta->level)][$course_meta->code] = $course_meta->name;
        }
        
        $form['course']['code'] = array('#type' => 'select',
                                        '#title' => t('Course'),
                                        '#options' => $courses,
                                        '#required' => TRUE);
    }
    
    $form['course']['program'] = array('#type' => 'textarea',
                                       '#title' => t('Program'),
                                       '#cols' => 50,
                                       '#rows' => 10,
                                       '#required' => TRUE,
                                       '#default_value' => $course ? $course->program : '');
    
    
    // Staff
    $staff = array();
    foreach (gauss_teaching_roles() as $role_id => $role_name) {
        $rid = db_result(db_query('SELECT rid FROM {role} WHERE name=\'%s\'', $role_id));
        
        $result = db_query('SELECT u.uid, u.name FROM {users} u INNER JOIN {users_roles} ur ON u.uid = ur.uid WHERE ur.rid = %d ORDER BY u.name',
                            $rid);
        while ($u = db_fetch_object($result)) {
            $staff[$role_name][$u->uid] = $u->name;
        }
    }
    
    $form['course']['staff'] = array('#type' => 'select',
                                     '#title' => t('Staff'),
                                     '#multiple' => TRUE,
                                     '#options' => $staff,
                                     '#default_value' => $course ? array_keys($course->staff['teachers'] + $course->staff['teaching assistants']) : '');
    
    $form['course']['submit'] = array('#type' => 'submit',
                                      '#value' => $course ? t('Save') : t('Add Course Offering'));

    if ($course) {
       $form['course']['delete'] = array('#type' => 'submit',
                                          '#value' => t('Delete course offering'),
                                          '#submit' => array('courses_form_delete_submit'));    
    }
    
    return $form;
}

function courses_form_submit($form, &$form_state) {
    $course = isset($form['#course']) ? $form['#course'] : NULL;
    $term = $form['#term'];
    $values = $form_state['values'];
    $course_meta = course_meta_load($course ? $course->code : $values['code']);
    $staff = $values['staff'];
    
    if ($course) {
        drupal_set_message(t('The course @code - @name has been updated.', array('@code' => $course->code,
                                                                                 '@name' => $course->name)));
        $form_state['redirect'] = 'math/courses/' . $course->cid;
        
        $course->program = $values['program'];
        drupal_write_record('courses', $course, 'cid');
        
        
        // Remove omited staff
        foreach (array_diff(array_keys($course->staff['teachers'] + $course->staff['teaching assistants']), array_keys($staff)) as $s_uid) {
            db_query('DELETE FROM {courses_staff} WHERE cid = %d AND uid = %d',
                     $course->cid, $s_uid);
            db_query('DELETE FROM {courses_staff_hours} WHERE cid = %d AND uid = %d',
                     $course->cid, $s_uid);                     
        }
        
        // Already existant staff is not added again
        foreach (array_keys($course->staff['teachers'] + $course->staff['teaching assistants']) as $s_uid) {
            if (isset($staff[$s_uid]))
                unset($staff[$s_uid]);
        }        
        
    } else {
        drupal_set_message(t('The course "@name" has been added to term @term.', array('@term' => $term->term,
                                                                                       '@name' => $course_meta->name)));
        $values['term'] = $term->term;
        drupal_write_record('courses', $values);
    }
    
    foreach ($staff as $staff_uid) {
        $reg = array('cid' => $course ? $course->cid : $values['cid'],
                     'uid' => $staff_uid);
        drupal_write_record('courses_staff', $reg);
    }
    
    cache_clear_all();
    menu_rebuild();
    
}

function courses_form_delete_submit($form, &$form_state) {
    $form_state['redirect'] = 'math/courses/' . arg(2) . '/delete';
}

function courses_delete_form($form_id, $course) {
    $form = array();
    $form['#course'] = $course;
    
    return confirm_form($form,
        t('Are you sure you want to delete the course "@code - @name"?', array('@code' => $course->code,
                                                                               '@name' => $course->name)),
        'math/courses/'. $course->cid, NULL, t('Delete'), t('Cancel'));
}

function courses_delete_form_submit($form, &$form_state) {
    $course = $form['#course'];
    
    db_query('DELETE FROM {courses} WHERE cid = %d', $course->cid);
    //db_query('DELETE FROM {courses_prereqs} WHERE cid = %d', $course->cid);
    db_query('DELETE FROM {courses_staff} WHERE cid = %d', $course->cid);
        
    cache_clear_all();
    menu_rebuild();

    drupal_set_message(t('The course and all its contents have been deleted.'));
    $form_state['redirect'] = 'math/courses/terms/' . $course->term;
}

function courses_admin_content($course) {
    $courses_node_types = courses_node_info();
    
    $output = '';
    
    $output .= '<dl>';
    foreach ($courses_node_types as $k => $type) {
        $node_type = node_get_types('type', $k);
        $output .= '<dt>';
        $output .= l($node_type->name, 'node/add/' . str_replace('_', '-', $node_type->type),
                    array('query' => array('cid' => $course->cid)));
        $output .= '</dt> <dd>' . filter_xss_admin($node_type->description) . '</dd>';
    }
    $output .= '</dl>';
    
    
    return $output;
}

function courses_admin_hours($course) {
    $output = '';
    
    $header = array(' ', t('Mon'), t('Tue'), t('Wed'), t('Thu'), t('Fri'), t('Sat'), t('Sun'));
    $rows = array();
    
    for ($i = 6; $i < 20; $i++) {
        $hd = array();
        
        foreach (array_keys(gauss_weekdays()) as $d) {
            $result = db_query('SELECT id, uid, description, notes FROM {courses_staff_hours} WHERE start <= %d AND end >= %d AND cid = %d AND day = \'%s\'',
                                $i, $i + 1, $course->cid, $d);
            while ($e = db_fetch_object($result)) {
                $e_user = user_load($e->uid);
                $hd[$d] .= '<div class="staff-hours-item"><span class="who">' .
                            check_plain($e_user->name) . '</span><br /><span class="what">' . check_plain($e->description) . '</span>' .
                            ($e->notes ? '<br /><span class="notes">' . $e->notes . '</span>' : '') .
                            l('[x]', 'math/courses/' . $course->cid . '/hours/delete/' . $e->id) .
                            '</div>';
            }
        }
        
        $rows[] = array($i . ' - ' . ($i + 1),
                        $hd['Mon'],
                        $hd['Tue'],
                        $hd['Wed'],
                        $hd['Thu'],
                        $hd['Fri'],
                        $hd['Sat'],
                        $hd['Sun']); 
    }
    
    $output .= theme('table', $header, $rows);
    
    if ($course->staff['teachers'] + $course->staff['teaching assistants'])
        $output .= drupal_get_form('courses_hours_add_form', $course);
    
    return $output;
}

function courses_hours_add_form($form_id, $course) {
    $staff_options = array();

    foreach (array('teachers' => t('Teachers'),
                   'teaching assistants' => t('Teaching Assistants')) as $k => $optgroup) {
        foreach ($course->staff[$k] as $s) {
            $staff_options[$optgroup][$s['uid']] = $s['name'];
        }
    }
    
    $form = array();
    $form['#course'] = $course;
    $form['hours'] = array('#type' => 'fieldset',
                           '#title' => t('Add staff hours'),
                           '#collapsible' => TRUE,
                           '#collapsed' => TRUE);
    $form['hours']['who'] = array('#type' => 'select',
                                  '#title' => t('Who'),
                                  '#options' => $staff_options,
                                  '#required' => TRUE);
    $form['hours']['what'] = array('#type' => 'textfield',
                                   '#title' => t('What'),
                                   '#maxlength' => 255,
                                   '#required' => TRUE);
    $form['hours']['notes'] = array('#type' => 'textarea',
                                    '#title' => t('Notes'),
                                    '#cols' => 20, '#rows' => 2);        
    
    $day_hours = array();
    for ($i = 6; $i < 20; $i++) {
        $day_hours[$i] = $i . ' - ' . ($i + 1);
    }
    
    foreach (gauss_weekdays() as $k => $v) {
        $form['hours']['when'][$k] = array('#type' => 'checkboxes',
                                           '#title' => $v,
                                           '#options' => $day_hours);
    }
    
    $form['hours']['submit'] = array('#type' => 'submit',
                                     '#value' => t('Save'));
    
    return $form;
}

function _courses_hours_group($day) {
    $ret = array();
    
    foreach ($day as $k => $d) {
        if (!$d) unset($day[$k]);
    }
    
    $day = array_keys($day);
    asort($day);
    
    for ($i = 0; $i < count($day); $i++) {
        $value = $day[$i];
        $ret_item['start'] = $value;
        $ret_item['end'] = $value + 1;
        
        for ($j = 1; $j < 24; $j++) {
            if (!in_array($value + $j, $day)) {
                break;
            }
            
            $ret_item['end'] = $value + $j + 1;
            $i = array_search($value + $j, $day);
        }
        
        $ret[] = $ret_item;
    }
    
    return $ret;
}

function courses_hours_add_form_submit($form, &$form_state) {
    $values = $form_state['values'];
    
    foreach (array_keys(gauss_weekdays()) as $d) {
        $grouped = _courses_hours_group($values[$d]);
        
        if ($grouped) {
            foreach ($grouped as $group) {
                $reg = array('cid' => $form['#course']->cid,
                             'uid' => $values['who'],
                             'description' => $values['what'],
                             'notes' => $values['notes'],
                             'day' => $d,
                             'start' => $group['start'],
                             'end' => $group['end']);
                
                drupal_write_record('courses_staff_hours', $reg);
            }
        }
    }
    
    drupal_set_message(t('Staff hours updated.'));
}

function courses_admin_hours_delete($course, $id) {
    db_query('DELETE FROM {courses_staff_hours} WHERE id = %d', $id);
    drupal_goto('math/courses/' . $course->cid . '/hours');
}

function courses_admin_exams($course) {
    $output = '';
    
    $header = array(t('Date'), t('Description'), t('Distribution'), t('Operations'));
    $rows = array();
    
    $res = db_query('SELECT e.id, e.description, e.date FROM {courses_exams} e WHERE e.cid = %d ORDER BY e.date', $course->cid);
    while ($ex = db_fetch_object($res)) {
        $exam = course_exam_load($ex->id);
        $rows[] = array(date_format(date_create($ex->date), 'd/M/Y'),
                        check_plain($ex->description),
                        $exam->is_programmed ? t('Yes') : t('No'), 
                        l(t('Edit'), 'math/courses/' . $course->cid . '/exams/' . $ex->id . '/edit') . ' | ' .
                        l(t('Distribution'), 'math/courses/' . $course->cid . '/exams/' . $ex->id . '/dist') . ' | ' .
                        l(t('Delete'), 'math/courses/' . $course->cid . '/exams/' . $ex->id . '/delete')
                        );
    }
    
    if (!$rows) $rows[] = array(array('data' => t('No entries found.'),
                                      'colspan' => 4));

    $output .= theme('table', $header, $rows);
    $output .= drupal_get_form('courses_exam_form');
    
    return $output;
}

function courses_exam_form($form_id, $exam = NULL) {
    $form = array();
    
    if ($exam) $form['#exam'] = $exam;
    
    $form['exam'] = array('#type' => 'fieldset',
                          '#title' => $exam ? t('Edit "@exam"', array('@exam' => $exam->description)) : 
                                      t('Program Exam'),
                          '#collapsible' => TRUE,
                          '#collapsed' => $exam ? FALSE : TRUE);
    $form['exam']['description'] = array('#type' => 'textfield',
                                         '#title' => t('Description'),
                                         '#maxlength' => 255,
                                         '#required' => TRUE,
                                         '#default_value' => $exam ? $exam->description : '');
    $form['exam']['date'] = array('#type' => 'date_select',
                                  '#title' => t('Date'),
                                  '#default_value' => $exam ? $exam->date : date('Y-m-d 00:00:00'),
                                  '#date_format' => 'd/M/Y',
                                  '#date_year_range' => '-0:+0',
                                  '#date_label_position' => 'within',
                                  '#required' => TRUE);
    $form['exam']['submit'] = array('#type' => 'submit',
                                    '#value' => t('Program exam'));
    return $form;
}

function courses_exam_form_submit($form, &$form_state) {
    $exam = $form_state['values'];
    
    if ($form['#exam']) {
        $_exam = $form['#exam'];
        $exam['id'] = $_exam->id;
        $exam['cid'] = $_exam->cid;
        
        drupal_set_message(t('The exam "@exam" has been updated.', array('@exam' => $exam['description'])));
        drupal_write_record('courses_exams', $exam, 'id');
        
        $form_state['redirect'] = 'math/courses/' . $_exam->cid . '/exams';
    } else {
        $exam['cid'] = arg(2);
        drupal_set_message(t('The exam "@exam" has been added to this course.', array('@exam' => $exam['description'])));
        drupal_write_record('courses_exams', $exam);
    }
}

function courses_exam_delete_form($form_id, $exam) {
    $form = array();
    $form['#exam'] = $exam;
    
    return confirm_form($form,
        t('Are you sure you want to delete the exam "@desc"?', array('@desc' => $exam->description)),
        'math/courses/'. $exam->cid . '/exams', NULL, t('Delete'), t('Cancel'));
}

function courses_exam_delete_form_submit($form, &$form_state) {
    $exam = $form['#exam'];
    
    db_query('DELETE FROM {courses_exams} WHERE id=%d', $exam->id);
    db_query('DELETE FROM {courses_exams_dist} WHERE exam_id=%d', $exam->id);
    
    cache_clear_all();
    menu_rebuild();
    
    drupal_set_message(t('The exam has been deleted.'));
    $form_state['redirect'] = 'math/courses/' . $exam->cid . '/exams';
}

function courses_admin_exam_dist($exam) {
    $coutput = '';
    
    $output .= drupal_get_form('courses_exam_dist_form', $exam);
    $output .= drupal_get_form('courses_exam_dist_detail_form', $exam);
    
    drupal_add_js(drupal_get_path('module', 'courses') . '/exam-details.js', 'module');
    
    return  $output;
}

function courses_exam_dist_form(&$form_state, $exam) {
    $form = array();
    $form['#exam'] = $exam;
    
    $f_same_time = isset($form_state['values']['same_time']) ? $form_state['values']['same_time'] : $exam->same_time;
    $f_same_place = isset($form_state['values']['same_place']) ? $form_state['values']['same_place'] : $exam->same_place;    
    
    $form['config'] = array('#type' => 'fieldset',
                            '#title' => t('General config.'),
                            '#collapsible' => FALSE);
    $form['config']['date_'] = array('#type' => 'textfield',
                                    '#title' => t('Date'),
                                    '#value' => date_format(date_create($exam->date), 'd/M/Y'),
                                    '#size' => 20,
                                    '#disabled' => TRUE);
    $form['config']['notes'] = array('#type' => 'textarea',
                                     '#title' => t('Notes'),
                                     '#cols' => 50, '#rows' => 3,
                                     '#default_value' => $exam->notes);
    
    $form['config']['same_time'] = array(
        '#type' => 'checkbox',
        '#title' => 'Todos los estudiantes presentan el examen al mismo tiempo.',
        '#default_value' => $f_same_time,
        '#ahah' => array('path' => 'math/courses/' . $exam->cid . '/exams/' . $exam->id . '/dist/ahah/same_time',
                        'wrapper' => 'exam-time',
                        'method' => 'replace',
                        'effect' => 'fade')    
    );
    if ($f_same_time) {
        $form['config']['same_time_date'] = array('#prefix' => '<div id="exam-time">',
                                                  '#suffix' => '</div>');

        $form['config']['same_time_date']['start'] = array(
            '#type' => 'date_select',
            '#title' => 'Hora inicio',
            '#date_format' => 'H:i',
            '#default_value' => $exam->start,
            '#date_label_position' => 'within');
        $form['config']['same_time_date']['end'] = array(
            '#type' => 'date_select',
            '#title' => 'Hora fin',
            '#date_format' => 'H:i',
            '#default_value' => $exam->end,
            '#date_label_position' => 'within');        
    } else {
        $form['config']['same_time_date'] = array('#value' => '<div id="exam-time"></div>');
    }
    
    $form['config']['same_place'] = array(
        '#type' => 'checkbox',
        '#title' => 'Todos los estudiantes presentan el examen en el mismo lugar.',
        '#default_value' => $f_same_place,
        '#ahah' => array('path' => 'math/courses/' . $exam->cid . '/exams/' . $exam->id . '/dist/ahah/same_place',
                        'wrapper' => 'exam-location',
                        'method' => 'replace',
                        'effect' => 'fade'));
    if ($f_same_place) {
        $form['config']['same_place_location'] = array('#type' => 'textfield',
                                                       '#title' => 'Salón',
                                                       '#prefix' => '<div id="exam-location">',
                                                       '#suffix' => '</div>',
                                                       '#default_value' => $exam->location,
                                                       '#size' => 10);
    } else {
        $form['config']['same_place_location'] = array('#value' => '<div id="exam-location"></div>');
    }
    
    $form['config']['submit'] = array('#type' => 'submit',
                                      '#value' => t('Save exam configuration')
    );
    
    return $form;
}

function courses_exam_dist_form_validate($form, &$form_state) {
    if ($form_state['values']['same_place']) {
        $loc = trim($form_state['values']['same_place_location']);
        
        if (empty($loc))
            form_set_error('same_place_location', 'Location is needed');
    }
    
    if ($form_state['values']['same_time']) {
        if (!isset($form_state['values']['start'])) {
            form_set_error('start', 'Start time is needed.');
        }
        
        if (!isset($form_state['values']['end'])) {
            form_set_error('end', 'End time is needed.');
        }        
    }
}

function courses_exam_dist_form_submit($form, &$form_state) {
    $exam = $form['#exam'];
    $exam->notes = $form_state['values']['notes'];
    $exam->same_time = $form_state['values']['same_time'];
    $exam->same_place = $form_state['values']['same_place'];
    
    if ($exam->same_time) {
        $start = date_part_extract($exam->date, 'year') . '-' .
                 date_part_extract($exam->date, 'month') . '-' .
                 date_part_extract($exam->date, 'day') . ' ' .
                 date_part_extract($form_state['values']['start'], 'hour') . ':' .
                 date_part_extract($form_state['values']['start'], 'minute') . ':00';
                 
        $end = date_part_extract($exam->date, 'year') . '-' .
                 date_part_extract($exam->date, 'month') . '-' .
                 date_part_extract($exam->date, 'day') . ' ' .
                 date_part_extract($form_state['values']['end'], 'hour') . ':' .
                 date_part_extract($form_state['values']['end'], 'minute') . ':00';

        $exam->start = $start;
        $exam->end = $end;
    } else {
        $exam->start = '';
        $exam->end = '';
    }
    
    if ($exam->same_place) {
        $exam->location = $form_state['values']['same_place_location'];
    } else {
        $exam->location = '';
    }
    
    drupal_set_message(t('Exam config. updated'));
    drupal_write_record('courses_exams', $exam, 'id');
    
    if ($exam->same_time && $exam->same_place) {
        db_query('DELETE FROM {courses_exams_dist} WHERE exam_id=%d', $exam->id);
    }
    
    if ($exam->same_time) {
        db_query('UPDATE {courses_exams_dist} SET start=\'%s\', end=\'%s\' WHERE exam_id=%d', $start, $end, $exam->id);
    }
    
    if ($exam->same_place) {
        db_query('UPDATE {courses_exams_dist} SET location=\'%s\' WHERE exam_id=%d', $exam->location, $exam->id);
    }

}

function _exam_detail_build($id, $exam, $criteria = '', $start = '', $end = '', $location = '') {
    $res = array();
    
    $res['criteria'] = array(
                '#type' => 'textarea',
                '#title' => t('Criterion'),
                '#cols' => 50, '#rows' => 1,
                '#default_value' => $criteria,
                '#prefix' => '<div class="exam-detail" id="exam-detail-' . $id . '">' 
    );
    
    if ($exam->same_time) {
        $res['start']['#value'] = '<b>' . t('Starting time') . ': </b>' . $exam->start . '<br />';
        $res['end']['#value'] = '<b>' . t('Ending time') . ': </b>' . $exam->end;
    } else {
        $res['start'] = array(
                    '#type' => 'date_select',
                    '#title' => t('Starting time'),
                    '#date_format' => 'H:i',
                    '#default_value' => $start,
                    '#date_label_position' => 'within');
                
        $res['end'] = array(
                '#type' => 'date_select',
                '#title' => t('Ending time'),
                '#date_format' => 'H:i',   
                '#default_value' => $end, 
                '#date_label_position' => 'within');
    }

    if ($exam->same_place) {
        $res['location']['#value'] = '<b>' . t('Location') . ': </b>' . $exam->location;
    } else {
        $res['location'] = array(
            '#type' => 'textfield',
            '#title' => t('Location'),
            '#size' => 10,
            '#default_value' => $exam->same_place ? $exam->location : $location,
            '#disabled' => $exam->same_place ? TRUE : FALSE);
    }
    
    $res['ops-' . $id] = array(
        '#value' => '<a href="#" onclick="delete_detail(\'' . $id . '\')">[' . t('delete') . ']</a> </div>',
    );
    
    return $res;
}

function courses_exam_dist_detail_form(&$form_state, $exam) {
    $form = array();
    $form['#exam_id'] = $exam->id;
    
    $form['dist'] = array('#type' => 'fieldset',
                          '#title' => t('Exam distribution'),
                          '#collapsible' => FALSE);
    
    if ($exam->same_place && $exam->same_time) {
        $form['dist']['not_configurable'] = array('#value' => t('There is no need for additional configuration.'));
    } else {
        
        if ($exam->distribution || isset($form_state['values']['details'])) {
            $form['dist']['details'] = array('#prefix' => '<div id="exam-details">', '#suffix' => '</div>', '#tree' => TRUE);
            
            foreach ($exam->distribution as $k => $v) {
                $form['dist']['details'][$k] = _exam_detail_build($k, $exam, $v->criteria, $v->start, $v->end, $v->location);
            }
            
        } else {
            $form['dist']['details'] = array('#value' => '<div id="exam-details"></div>', '#tree' => TRUE);
        }
        
        if (isset($form_state['values']['details'])) {
            foreach ($form_state['values']['details'] as $k => $v) {
                $form['dist']['details'][$k] = _exam_detail_build($k, $exam, $v['criteria'], $v['start'], $v['end'], $v['location']);
            }
        }
        
        $form['dist']['more'] = array(
            '#type' => 'button',
            '#value' => t('Add criterion'),
            '#ahah' => array(
                'path' => 'math/courses/' . $exam->cid . '/exams/' . $exam->id . '/dist/ahah/criteria',
                'wrapper' => 'exam-details',
                'method' => 'replace',
                'effect' => 'fade'
            )
        );

        if (isset($form_state['values']['more'])) {
            $id = time();
            $form['dist']['details'][$id] = _exam_detail_build($id, $exam);
        }
        
        $form['dist']['submit'] = array('#type' => 'submit',
                                        '#value' => t('Save exam distribution'));
    }
    
    return $form;
}

function courses_exam_dist_detail_form_submit($form, &$form_state) {
    $exam_ = course_exam_load($form['#exam_id']);
    
    db_query('DELETE FROM {courses_exams_dist} WHERE exam_id=%d', $exam_->id);

    if (!$exam_->same_place || !$exam_->same_time) {
        if (isset($form_state['values']['details'])) {
            foreach ($form_state['values']['details'] as $did => $detail) {
                foreach ($detail as $k => $v) $detail[$k] = trim($detail[$k]);
                
                if (($exam_->same_place && !empty($detail['criteria']) &&
                                      !empty($detail['start']) &&
                                      !empty($detail['end'])) ||
                    ($exam_->same_time && !empty($detail['criteria']) &&
                                          !empty($detail['location'])) ||
                    (!empty($detail['criteria']) &&
                     !empty($detail['location']) &&
                     !empty($detail['start']) &&
                     !empty($detail['end'])) ) {

                    $detail['exam_id'] = $exam_->id;
                    
                    if ($exam_->same_time) {
                        $detail['start'] = $exam_->start;
                        $detail['end'] = $exam_->end;
                    } else {
                        $detail['start'] = date_part_extract($exam_->date, 'year') . '-' .
                                 date_part_extract($exam_->date, 'month') . '-' .
                                 date_part_extract($exam_->date, 'day') . ' ' .
                                 date_part_extract($detail['start'], 'hour') . ':' .
                                 date_part_extract($detail['start'], 'minute') . ':00';
                                 
                        $detail['end'] = date_part_extract($exam_->date, 'year') . '-' .
                                 date_part_extract($exam_->date, 'month') . '-' .
                                 date_part_extract($exam_->date, 'day') . ' ' .
                                 date_part_extract($detail['end'], 'hour') . ':' .
                                 date_part_extract($detail['end'], 'minute') . ':00';
                    }
                    
                    if ($exam_->same_place) {
                        $detail['location'] = $exam_->location;
                    }
                    
                    drupal_write_record('courses_exams_dist', $detail);
                }
    
            }
        }
    }
    
    drupal_set_message(t('Exam distribution updated.'));
}


function courses_admin_exam_dist_ahah($op = NULL) {
    $form_state = array('storage' => NULL, 'submitted' => FALSE);
    $form_build_id = $_POST['form_build_id'];
    
    $form = form_get_cache($form_build_id, $form_state);
    
    $args = $form['#parameters'];
    $form_id = array_shift($args);
    $form_state['post'] = $form['#post'] = $_POST;
    $form['#programmed'] = $form['#redirect'] = FALSE;
    
    drupal_process_form($form_id, $form, $form_state);
    $form = drupal_rebuild_form($form_id, $form_state, $args, $form_build_id);
    
    switch ($op) {
        case 'same_place':
            $form_mod = $form['config']['same_place_location'];
            unset($form_mod['#prefix'], $form_mod['#suffix']);
            break;
        case 'same_time':
            $form_mod = $form['config']['same_time_date'];
            unset($form_mod['#prefix'], $form_mod['#suffix']);
            break;
        case 'criteria':
            //$id = $form['#last_id'];
            $form_mod = $form['dist']['details'];
            //unset($form['#last_id']);
            unset($form_mod['#prefix'], $form_mod['#suffix']);
            break;                   
        default:
            break;
    }
    
    $output = theme('status_messages') . drupal_render($form_mod);
    
    drupal_json(array('status' => TRUE, 'data' => $output));
}