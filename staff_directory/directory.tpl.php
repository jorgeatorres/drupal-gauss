
<ul class="staff-directory-categories">
    <?php foreach ($categories as $_cid => $name): ?>
    <li class="<?php echo $cid == $_cid ? 'active' : ''; ?>"><?php echo l($name, 'dir/' . $_cid); ?></li>
    <?php endforeach; ?>
</ul>

<?php echo $results_table; ?>
