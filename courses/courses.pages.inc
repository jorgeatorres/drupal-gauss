<?php
function _die_404() {
    drupal_not_found(); exit;
}

function courses_page_level($level = 'undergraduate') {
    $output = '';
    
    $output .= courses_page_term(courses_current_term(),
                                 $level);
    
    return $output;
}

function courses_page_term($term_id, $level = NULL) {
    $term = courses_term_load($term_id) or _die_404();

    drupal_set_breadcrumb(array(l(t('Home'), '<front>'),
                                l(t('Courses'), 'cur')));    

    $output = '';
    $output .= '<h3>' . t('Term @term', array('@term' => $term->term)) . '</h3>';
    
    $header = array(t('Course Code'), t('Course'));

    $courses = array();
    if (!$level) {
        $header[] = t('Level');
        
        if (isset($term->courses['undergraduate']))
            $courses += $term->courses['undergraduate'];
            
        if (isset($term->courses['graduate']))
            $courses += $term->courses['graduate'];            
    } else {
        if (isset($term->courses[$level]))
            $courses += $term->courses[$level];
    }
    
    if ($courses) {
        foreach ($courses as $cid => $course) {
            if (!$level) {
                $rows[] = array($course->code,
                                l(check_plain($course->name), 'cur/' . $term->term . '/' . $course->code),
                                courses_levels($course->level));
            } else {
                $rows[] = array($course->code,
                                l(check_plain($course->name), 'cur/' . $term->term . '/' . $course->code));                
            }
        }
    } else {
        $rows[] = array(array('data' => t('No courses found.'), 'colspan' => !$level ? 2 : 3));
    }
    
    $output .= theme('table', $header, $rows);
    

    return $output;
}

function courses_page_course($cid) {
    $course = course_load($cid) or _die_404();
    courses_set_breadcrumb($course);
    
    $output = '';
    
    /* Basic info */
    $output .= '<div class="course-page-info">';
    $output .= '<dl>';
    
    $output .= '<dt>' . t('Level') . '</dt>';
    $output .= '<dd>' . courses_levels($course->level) . '</dd>';
    
    $output .= '<dt>' . t('Code') . '</dt>';
    $output .= '<dd>' . $course->code . '</dd>';
    
    $output .= '<dt>' . t('Term') . '</dt>';
    $output .= '<dd>' . $course->term . '</dd>';    
    
    $output .= '<dt>' . t('Teachers') . '</dt>';
    if ($course->staff['teachers']) {
        $output .= '<dd><ul class="staff-list">';
        
        foreach ($course->staff['teachers'] as $teacher) {
            $teacher_user = user_load($teacher['uid']);
            
            $output .= '<li>';
            $output .= $teacher_user->name . ' (';
            $output .= l($teacher_user->mail, 'mailto:' . $teacher_user->mail) . ') <br/>';
            if (!empty($teacher_user->profile->office))
                $output .= '<span class="office">' . t('Office @office', array('@office' => $teacher_user->profile->office)) . '</span>';
            
            $output .= '</li>';
        }
        
        $output .= '</ul></dd>';
    } else {
        $output .= '<dd>--</dd>';
    }
    
    $output .= '<dt>' . t('Teaching Assistants') . '</dt>';
    if ($course->staff['teaching assistants']) {
        $output .= '<dd><ul class="staff-list">';
        
        foreach ($course->staff['teaching assistants'] as $ta) {
            $ta_user = user_load($ta['uid']);
            
            $output .= '<li>';
            $output .= $ta_user->name . ' (' . l($ta_user->mail, 'mailto:' . $ta_user->mail) . ')';
            $output .= '</li>';
        }
        
        $output .= '</ul></dd>';
    } else {
        $output .= '<dd>--</dd>';
    }
    $output .= '</dl>';
    
    $output .= '</div>';

    $recent_news = array();
    $result = db_query('SELECT DISTINCT cn.nid, n.type FROM {courses_nodes} cn INNER JOIN {node} n ON n.nid = cn.nid WHERE n.type = \'%s\' AND cn.cid = %d ORDER BY cn.nid DESC LIMIT 5',
                       'course_news', $course->cid);
    while ($item_ = db_fetch_object($result)) {
        $item = node_load($item_->nid);
        $recent_news[] = $item;
    }
    
    $next_exams = array();
    $result = db_query('SELECT e.id, e.description, e.date FROM {courses_exams} e WHERE e.cid = %d AND e.date >= \'%s\' ORDER BY e.date LIMIT 3',
                       $course->cid, date(DATE_FORMAT_DATETIME));
    while ($exam_ = db_fetch_object($result)) {
        $next_exams[] = $exam_;
    }
    
    $output .= '<div class="course-page-extra">';
    
        $output .= '<div class="col1">';
        $output .= '<div class="course-news">';
        $output .= '<h3>' . t('News') . '</h3>';
        if ($recent_news) {
            $output .= '<ul>';
            
            foreach ($recent_news as $item) {
                $output .= '<li>';
                $output .= '<span class="date">' . date('d/M/Y H:i', $item->created) . '</span>';
                $output .= l(check_plain($item->title), 'node/' . $item->nid);
                $output .= '<div class="teaser">' . $item->teaser . '</div>';
                $output .= '</li>';
            }

            $output .= '</ul>';
        } else {
            $output .= '<p>' . t('There are no recent news items.') . '</p>';
        }
        $output .= '</div>';
        
        $output .= '</div>';
    
        $output .= '<div class="col2">';
        $output .= '<div class="course-upcoming-exams">';
        $output .= '<h3>' . t('Upcoming exams') . '</h3>';
        if ($next_exams) {
            $output .= '<ul>';
            
            foreach ($next_exams as $exam) {
                $output .= '<li>';
                $output .= '<span class="date">' . date_format(date_create($exam->date), 'd/M/Y') . '</span>';
                $output .= l(check_plain($exam->description),
                             'cur/' . $course->term . '/' . $course->code . '/eval/' . $exam->id);
                $output .= '</li>';
            }
            
            $output .= '</ul>';
        } else {
            $output .= '<p>' . t('There are no upcoming exams in this course.') . '</p>';
        }
        $output .= '</div>';
        
        $output .= '</div>';    
    
    $output .= '</div>';
    
    return $output;
}

function courses_page_program($cid) {
    $course = course_load($cid) or _die_404();
    courses_set_breadcrumb($course);
    
    return check_markup($course->program);
}

function courses_page_content_problem_sets($cid) {
    $course = course_load($cid) or _die_404();
    courses_set_breadcrumb($course);

    $output = '';
    
    $header = array(t('Problem Set'), t('Solution'));
    $rows = array();
    
    $result = db_query('SELECT DISTINCT cn.nid, n.type FROM {courses_nodes} cn INNER JOIN {node} n ON n.nid = cn.nid WHERE n.type = \'%s\' AND cn.cid = %d',
                       'problem_set',
                       $course->cid);
    
    while ($node_ = db_fetch_object($result)) {
        $node = node_load($node_->nid);
        
        $row = array();
        $href = file_create_url($node->problem_set_file->filepath);
        $row[] = l($node->title, $href) . ' (' . l(t('download'), $href) . ')';
        
        if (isset($node->solution_file)) {
            $row[] = l(t('Download solution'), file_create_url($node->solution_file->filepath));
        } else {
            $row[] = '--';
        }
        
        $rows[] = $row;
    }
    
    if (!$rows) $rows[] = array(array('data' => t('No problem sets found.'),
                                      'colspan' => 2));
    
    $output .= theme('table', $header, $rows);
    
    return $output;
}

function courses_page_content_exams($cid) {
    $course = course_load($cid) or _die_404();
    courses_set_breadcrumb($course);

    $output = '';
    
    $header = array(t('Exam'), t('Solution'));
    $rows = array();
    
    $result = db_query('SELECT DISTINCT cn.nid, n.type FROM {courses_nodes} cn INNER JOIN {node} n ON n.nid = cn.nid WHERE n.type = \'%s\' AND cn.cid = %d',
                       'exam',
                       $course->cid);
    
    while ($node_ = db_fetch_object($result)) {
        $node = node_load($node_->nid);
        
        $row = array();
        $href = file_create_url($node->exam_file->filepath);
        $row[] = l($node->title, $href) . ' (' . l(t('download'), $href) . ')';
        
        if (isset($node->solution_file)) {
            $row[] = l(t('Download solution'), file_create_url($node->solution_file->filepath));
        } else {
            $row[] = '--';
        }
        
        $rows[] = $row;
    }
    
    if (!$rows) $rows[] = array(array('data' => t('No exams found.'),
                                      'colspan' => 2));
    
    $output .= theme('table', $header, $rows);
    
    return $output;
}

function courses_page_content_notes($cid) {
    $course = course_load($cid) or _die_404();
    courses_set_breadcrumb($course);

    $output = '';
    
    $header = array(t('Class Note'), '');
    $rows = array();
    
    $result = db_query('SELECT DISTINCT cn.nid, n.type FROM {courses_nodes} cn INNER JOIN {node} n ON n.nid = cn.nid WHERE n.type = \'%s\' AND cn.cid = %d',
                       'class_notes',
                       $course->cid);
    
    while ($node_ = db_fetch_object($result)) {
        $node = node_load($node_->nid);

        $rows[] = array(l(check_plain($node->title),
                          'node/' . $node->nid),
                        l(t('view'), 'node/' . $node->nid));
        
    }
    
    if (!$rows) $rows[] = array(array('data' => t('No class notes found.'),
                                      'colspan' => 2));
    
    $output .= theme('table', $header, $rows);
    
    return $output;
}

function courses_page_exams($cid) {
    $course = course_load($cid) or _die_404();
    courses_set_breadcrumb($course);

    $output = '';
    
    $header = array(t('Date'), t('Exam'), '');
    $rows = array();

    $result = db_query('SELECT e.id, e.description, e.date FROM {courses_exams} e WHERE e.cid = %d ORDER BY e.date',
                       $course->cid);
    while ($exam_ = db_fetch_object($result)) {
        $exam = course_exam_load($exam_->id);
        $href = 'cur/' . $course->term . '/' . $course->code . '/eval/' . $exam->id;

        $rows[] = array(date_format(date_create($exam->date), 'd/M/Y'),
                        l(check_plain($exam->description), $href),
                        l(t('View details'), $href));
    }
    
    if (!$rows) $rows[] = array(array('data' => t('No exams programmed.'),
                                      'colspan' => 3));
    
    $output .= theme('table', $header, $rows);
    return $output;
}

function courses_page_exam($cid, $exam) {
    $course = course_load($cid) or _die_404();
    courses_set_breadcrumb($course,
                          array(array('Programación de evaluaciones',
                                      'cur/' . $course->term . '/' . $course->code . '/eval')));    
    
    $output = '';
    
    $output .= '<dl class="exam-details">';
    $output .= '<dt>' . t('Date') . '</dt>';
    $output .= '<dd>' . date_format(date_create($exam->date), 'd/M/Y') . '</dd>';
    $output .= '<dt>' . t('Notes') . '</dt>';
    $output .= '<dd>' . (!empty($exam->notes) ? check_markup($exam->notes) : '--') . '</dd>';
    $output .= '<dt>' . t('Distribution') . '</dt>';
    
    if (!$exam->is_programmed) {
        $output .= '<dd>--</dd>';
    } else {
        $output .= '<dd>';
        
        if ($exam->same_time && $exam->same_place) {
            $output .= '<b>' . t('Time') . ':</b> ' . date_format(date_create($exam->start), 'h:i A');
            $output .=  ' - ' . date_format(date_create($exam->end), 'h:i A') . '<br />';
            $output .= '<b>' . t('Location') . ':</b> ' . check_plain($exam->location);
        } else {
            $header = array('', t('Time'), t('Location'));
            $rows = array();
            
            foreach ($exam->distribution as $dist) {
                $rows[] = array(check_plain($dist->criteria),
                                date_format(date_create($dist->start),
                                            'h:i A') . ' - ' .
                                date_format(date_create($dist->end),
                                            'h:i A'),
                                check_plain($dist->location));
            }
            
            $output .= theme('table', $header, $rows);
        }
        
        $output .= '</dd>';
    }
    
    $output .= '</dl>';

    return $output;
}

function courses_page_hours($cid) {
    $course = course_load($cid) or _die_404();
    courses_set_breadcrumb($course);
    
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

    $output .= theme('table', $header, $rows, array('id' => 'course-staff-hours'));    
    
    return $output;
}