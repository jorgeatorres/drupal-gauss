<div class="directory-profile">

	<h2><?php echo $account->profile->name; ?></h2>

	<?php if ($account->profile->title): ?>
		<span class="profile-title"><?php echo $account->profile->title; ?></span>
	<?php endif; ?>

	<div class="info-wrapper clearfix">
		<div class="col1">
			<dl>
				<dt><?php echo t('Email'); ?></dt>
				<dd><?php echo $account->mail ? $account->mail : '--'; ?></dd>	

				<dt><?php echo t('Office'); ?></dt>
				<dd><?php echo $account->profile->office ? $account->profile->office : '--'; ?></dd>

				<dt><?php echo t('Extension'); ?></dt>
				<dd><?php echo $account->profile->extension ? $account->profile->extension : '--'; ?></dd>	

        <dt><?php echo t('WebPage'); ?></dt>
        <dd><?php echo $account->profile->links ? '<a href="' . $account->profile->links . '" target="_blank">' .  $account->profile->links . '</a>' : '--'; ?></dd>

				<dt><?php echo t('Bio'); ?></dt>
				<dd><?php echo $account->profile->bio ? check_markup($account->profile->bio, 'filtered_html') : '--'; ?></dd>

				<dt><?php echo t('Interests'); ?></dt>
				<dd><?php echo $account->profile->interests ? check_markup($account->profile->interests, 'filtered_html') : '--'; ?></dd>	
			</dl>		
		</div>
		<div class="col2"><?php print theme('user_picture', array('account' => $account));?></div>
	</div>

</div>
