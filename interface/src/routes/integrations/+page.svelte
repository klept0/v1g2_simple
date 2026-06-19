<script>
	import PageHeader from '$lib/components/PageHeader.svelte';
	import StatusAlert from '$lib/components/StatusAlert.svelte';
	import SettingsObdCard from '$lib/features/settings/SettingsObdCard.svelte';

	let message = $state(null);
</script>

<div class="page-stack">
	<PageHeader title="Integrations" subtitle="OBD connectivity and speed source status." />

	<StatusAlert {message} />

	<SettingsObdCard />

	<!-- JBV1 Integration -->
	<div class="card surface-card">
		<div class="card-body gap-4">
			<h2 class="card-title">JBV1 — GPS &amp; Speed Display</h2>
			<p class="copy-body">
				The JBV1 screen on v1simple shows speed, GPS accuracy, and heading pulled live from your
				Android phone via <strong>Tasker</strong>. Data is pushed over Wi-Fi every 2 seconds and
				expires after 10 s of no updates.
			</p>

			<div class="divider my-1">Requirements</div>
			<ul class="list-disc list-inside copy-body space-y-1">
				<li>Android phone with <strong>Tasker</strong> installed.</li>
				<li>Phone connected to the <strong>v1simple Wi-Fi hotspot</strong> (check SSID/password on the <a href="/settings" class="link link-primary">Settings</a> page).</li>
				<li>Android location permission granted to Tasker.</li>
			</ul>

			<div class="divider my-1">Tasker Setup</div>

			<div class="alert alert-info text-sm py-2">
				<svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" class="stroke-current shrink-0 w-5 h-5"><path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M13 16h-1v-4h-1m1-4h.01M21 12a9 9 0 11-18 0 9 9 0 0118 0z"></path></svg>
				<span>Want to skip the manual steps? <a href="https://github.com/klept0/v1g2_simple/raw/main/docs/v1simple_jbv1_tasker.prf.xml" class="link font-semibold" target="_blank">Download the Tasker profile</a> and import it via <strong>Tasker → ⋮ → Data → Restore</strong>. Change the SSID inside the file if your network name differs from <code>v1simple</code>.</span>
			</div>

			<p class="copy-caption font-semibold">Step 1 — Create a Profile (auto-start on v1simple Wi-Fi)</p>
			<ol class="list-decimal list-inside copy-body space-y-1 ml-2">
				<li>Tap <strong>+</strong> → <strong>State</strong> → <strong>Net</strong> → <strong>Wi-Fi Connected</strong></li>
				<li>Set <strong>SSID</strong> to your v1simple network name</li>
				<li>Back arrow → <strong>New Task</strong> → name it <code>v1simple GPS Push</code></li>
			</ol>

			<p class="copy-caption font-semibold mt-2">Step 2 — Build the task (add actions in order)</p>
			<div class="overflow-x-auto">
				<table class="table table-sm text-sm">
					<thead><tr><th>#</th><th>Action</th><th>Settings</th></tr></thead>
					<tbody>
						<tr><td>1</td><td>Variable Set</td><td><code>%LOCSPEED_MPH</code> = <code>%LOCSPEED * 2.23694</code> &nbsp;☑ Do Maths</td></tr>
						<tr><td>2</td><td>Variable Set</td><td><code>%LOCACC_M</code> = <code>%LOCACC</code></td></tr>
						<tr><td>3</td><td>Net → HTTP Request</td><td>See below</td></tr>
						<tr><td>4</td><td>Task → Wait</td><td>2 seconds</td></tr>
						<tr><td>5</td><td>Task → Go To</td><td>Action #1 (loops indefinitely)</td></tr>
					</tbody>
				</table>
			</div>

			<p class="copy-caption font-semibold mt-2">Step 3 — HTTP Request action config</p>
			<ul class="list-disc list-inside copy-body space-y-1 ml-2">
				<li><strong>Method:</strong> POST</li>
				<li><strong>URL:</strong> <code>http://192.168.4.1/api/jbv1/update</code></li>
				<li><strong>Content Type:</strong> <code>application/json</code></li>
				<li><strong>Body:</strong></li>
			</ul>
			<pre class="bg-base-200 rounded p-2 text-sm overflow-x-auto ml-2">{`{"speed":%LOCSPEED_MPH,"heading":"%LOCBEAR","gpsAccuracy":%LOCACC_M,"satellites":0}`}</pre>

			<p class="copy-caption font-semibold mt-2">Step 4 — Exit Task (clean up on disconnect)</p>
			<ol class="list-decimal list-inside copy-body space-y-1 ml-2">
				<li>Tap the profile → <strong>Exit Task</strong> → <strong>New Task</strong></li>
				<li>Add: <strong>Task → Stop</strong> → check <em>All tasks named</em> → <code>v1simple GPS Push</code></li>
			</ol>

			<div class="divider my-1">What you get</div>
			<ul class="list-disc list-inside copy-body space-y-1">
				<li><strong>Speed</strong> — converted from Android GPS (m/s → MPH)</li>
				<li><strong>Heading</strong> — GPS bearing in degrees (<code>%LOCBEAR</code>); displayed as-is</li>
				<li><strong>GPS Accuracy</strong> — horizontal accuracy in metres</li>
			</ul>
			<p class="copy-caption">
				<strong>Speed limit</strong> is not available — JBV1 knows it internally but does not share it with Tasker or any external app.
				The delta field on the JBV1 screen will remain blank until a speed limit source is available.
				Satellite count requires the <strong>AutoLocation</strong> Tasker plugin; without it the field shows 0.
			</p>

			<div class="divider my-1">API Reference</div>
			<p class="copy-caption">The endpoint accepts a JSON POST. All fields are optional — only present fields are updated.</p>
			<pre class="bg-base-200 rounded p-2 text-sm overflow-x-auto">{`POST /api/jbv1/update
Content-Type: application/json

{
  "speed": 72,        // integer MPH
  "speedLimit": 65,   // integer MPH (optional — omit if unknown)
  "heading": "NW",    // string or bearing degrees
  "gpsAccuracy": 3,   // integer metres
  "satellites": 8     // integer
}`}</pre>
			<p class="copy-caption">Returns <code>{`{"ok":true}`}</code> on success. Data expires after <strong>10 seconds</strong> of no updates.</p>
		</div>
	</div>
</div>
