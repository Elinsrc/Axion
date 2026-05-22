package su.xash.axion;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Bundle;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import androidx.appcompat.app.AppCompatActivity;
import com.google.android.material.button.MaterialButton;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;
import org.json.JSONArray;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;

public class MainActivity extends AppCompatActivity {
    private SharedPreferences prefs;
    private static final String PREF_NAME = "AxionPrefs";
    private static final String KEY_ARGV = "last_argv";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        prefs = getSharedPreferences(PREF_NAME, MODE_PRIVATE);

        androidx.appcompat.widget.Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        EditText argvInput = findViewById(R.id.argvInput);
        Button runButton = findViewById(R.id.runButton);
        Button infoButton = findViewById(R.id.infoButton);
        MaterialButton updateButton = findViewById(R.id.updateButton);

        String defaultArgs = argvInput.getText().toString(); 
        argvInput.setText(prefs.getString(KEY_ARGV, defaultArgs));

        argvInput.setSingleLine(true);
        argvInput.setImeOptions(EditorInfo.IME_ACTION_DONE);
        argvInput.setOnEditorActionListener((v, actionId, event) -> {
            if (actionId == EditorInfo.IME_ACTION_DONE) {
                saveArgs(argvInput.getText().toString());
                argvInput.clearFocus();
                InputMethodManager imm = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
                if (imm != null) {
                    imm.hideSoftInputFromWindow(v.getWindowToken(), 0);
                }
                return true;
            }
            return false;
        });

        runButton.setOnClickListener(v -> {
            String userArgs = argvInput.getText().toString();
            saveArgs(userArgs);
            startGame(userArgs.trim());
        });

        infoButton.setOnClickListener(v -> {
            startActivity(new Intent(MainActivity.this, GitHubActivity.class));
        });

        updateButton.setOnClickListener(v -> {
            startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(GitHubActivity.DOWNLOAD_URL)));
        });

        checkUpdates(BuildConfig.COMMIT_HASH);
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (GitHubActivity.sTestMode) {
            showUpdateInfo("aec0789150e64b5b7ac1b88625353bab473695c3", "Test latest commit message");
        }
    }

    private void showUpdateInfo(String hash, String message) {
        findViewById(R.id.updateStatusText).setVisibility(View.VISIBLE);
        
        View latestCommitArea = findViewById(R.id.latestCommitArea);
        TextView latestCommitHash = findViewById(R.id.latestCommitHash);
        TextView latestCommitMessage = findViewById(R.id.latestCommitMessage);
        MaterialButton updateButton = findViewById(R.id.updateButton);
        
        latestCommitHash.setText(getString(R.string.commit_hash, hash));
        latestCommitMessage.setText(message);
        
        latestCommitArea.setVisibility(View.VISIBLE);
        updateButton.setVisibility(View.VISIBLE);
        
        latestCommitArea.setOnClickListener(v -> startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(GitHubActivity.DOWNLOAD_URL))));
    }

    private void checkUpdates(String currentHash) {
        if (GitHubActivity.sTestMode) {
            showUpdateInfo("aec0789150e64b5b7ac1b88625353bab473695c3", "Test latest commit message");
            return;
        }

        new Thread(() -> {
            try {
                URL url = new URL(GitHubActivity.COMMITS_API);
                HttpURLConnection conn = (HttpURLConnection) url.openConnection();
                conn.setRequestMethod("GET");
                conn.setRequestProperty("User-Agent", "Axion-App");

                BufferedReader reader = new BufferedReader(new InputStreamReader(conn.getInputStream()));
                StringBuilder responseBuilder = new StringBuilder();
                String line;
                while ((line = reader.readLine()) != null) {
                    responseBuilder.append(line);
                }
                reader.close();

                JSONArray commits = new JSONArray(responseBuilder.toString());
                boolean outdated = false;
                String latestHash = "";
                String latestMsg = "";
                
                if (commits.length() > 0) {
                    latestHash = commits.getJSONObject(0).getString("sha");
                    latestMsg = commits.getJSONObject(0).getJSONObject("commit").getString("message");
                    if (!latestHash.equals(currentHash)) {
                        outdated = true;
                    }
                }

                final boolean finalOutdated = outdated;
                final String finalLatestHash = latestHash;
                final String finalLatestMsg = latestMsg;
                runOnUiThread(() -> {
                    if (finalOutdated || GitHubActivity.sTestMode) {
                        showUpdateInfo(finalLatestHash, finalLatestMsg);
                    } else {
                        findViewById(R.id.updateStatusText).setVisibility(View.GONE);
                        findViewById(R.id.latestCommitArea).setVisibility(View.GONE);
                        findViewById(R.id.updateButton).setVisibility(View.GONE);
                    }
                });

            } catch (Exception ignored) {}
        }).start();
    }

    private void saveArgs(String args) {
        prefs.edit().putString(KEY_ARGV, args).apply();
    }

    private void startGame(String argv) {
        String pkg = "su.xash.engine.test";

        try {
            getPackageManager().getPackageInfo(pkg, 0);
        } catch (PackageManager.NameNotFoundException e) {
            try {
                pkg = "su.xash.engine";
                getPackageManager().getPackageInfo(pkg, 0);
            } catch (PackageManager.NameNotFoundException ex) {
                new MaterialAlertDialogBuilder(this)
                        .setTitle(R.string.engine_not_found_title)
                        .setMessage(R.string.engine_not_found_msg)
                        .setPositiveButton(android.R.string.yes, (dialog, which) -> {
                            startActivity(new Intent(Intent.ACTION_VIEW,
                                    Uri.parse("https://github.com/FWGS/xash3d-fwgs/releases/tag/continuous")));
                        })
                        .setNegativeButton(android.R.string.no, null)
                        .show();
                return;
            }
        }

        startActivity(new Intent().setComponent(new ComponentName(pkg, "su.xash.engine.XashActivity"))
                .putExtra("gamedir", "valve")
                .putExtra("gamelibdir", getApplicationInfo().nativeLibraryDir)
                .putExtra("argv", argv)
                .putExtra("package", getPackageName()));
    }
}
