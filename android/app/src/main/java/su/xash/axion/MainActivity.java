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

import com.google.android.material.dialog.MaterialAlertDialogBuilder;

public class MainActivity extends AppCompatActivity {

    private static final String PREF_NAME = "AxionPrefs";
    private static final String KEY_ARGV = "last_argv";
    private static final String ENGINE_ACTIVITY = "su.xash.engine.XashActivity";
    private static final String ENGINE_DOWNLOAD_URL = "https://github.com/FWGS/xash3d-fwgs/releases/tag/continuous";
    private static final String[] ENGINE_PACKAGES = {"su.xash.engine.test", "su.xash.engine"};
    private static final String TEST_HASH = "aec0789150e64b5b7ac1b88625353bab473695c3";
    private static final String TEST_MSG = "Test message";

    private SharedPreferences prefs;
    private EditText argvInput;
    private Button runButton;
    private TextView statusText;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        prefs = getSharedPreferences(PREF_NAME, MODE_PRIVATE);
        setSupportActionBar(findViewById(R.id.toolbar));

        argvInput = findViewById(R.id.argvInput);
        runButton = findViewById(R.id.runButton);
        statusText = findViewById(R.id.updateStatusText);

        argvInput.setText(prefs.getString(KEY_ARGV, argvInput.getText().toString()));

        argvInput.setOnEditorActionListener((v, actionId, event) -> {
            if (actionId == EditorInfo.IME_ACTION_DONE) {
                saveArgs(argvInput.getText().toString());
                argvInput.clearFocus();
                hideKeyboard(v);
                return true;
            }
            return false;
        });

        runButton.setOnClickListener(v -> {
            String args = argvInput.getText().toString().trim();
            saveArgs(args);
            startGame(args);
        });

        findViewById(R.id.infoButton).setOnClickListener(v ->
                startActivity(new Intent(this, UpdateActivity.class)));

        findViewById(R.id.updateButton).setOnClickListener(v ->
                openUrl(UpdateActivity.DOWNLOAD_URL));

        performUpdateCheck();
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (UpdateActivity.sTestMode) UpdateActivity.showUpdateInfo(this, TEST_HASH, TEST_MSG);
    }

    private void performUpdateCheck() {
        if (UpdateActivity.sTestMode) {
            UpdateActivity.showUpdateInfo(this, TEST_HASH, TEST_MSG);
            return;
        }

        setRunButtonEnabled(false);
        statusText.setText(R.string.update_check);
        statusText.setVisibility(View.VISIBLE);

        UpdateActivity.checkUpdates(this, BuildConfig.COMMIT_HASH, (success, outdated, hash, msg) -> {
            setRunButtonEnabled(true);

            if (!success) {
                setStatus(R.string.update_error, android.R.color.holo_red_light);
                return;
            }

            if (outdated) {
                UpdateActivity.showUpdateInfo(this, hash, msg);
            } else {
                setStatus(R.string.up_to_date, android.R.color.holo_green_light);
                findViewById(R.id.latestCommitArea).setVisibility(View.GONE);
                findViewById(R.id.updateButton).setVisibility(View.GONE);
            }
        });
    }

    private void startGame(String argv) {
        String pkg = findEnginePackage();

        if (pkg == null) {
            new MaterialAlertDialogBuilder(this)
                    .setTitle(R.string.engine_not_found_title)
                    .setMessage(R.string.engine_not_found_msg)
                    .setPositiveButton(android.R.string.yes, (d, w) -> openUrl(ENGINE_DOWNLOAD_URL))
                    .setNegativeButton(android.R.string.no, null)
                    .show();
            return;
        }

        startActivity(new Intent()
                .setComponent(new ComponentName(pkg, ENGINE_ACTIVITY))
                .putExtra("gamedir", "valve")
                .putExtra("gamelibdir", getApplicationInfo().nativeLibraryDir)
                .putExtra("argv", argv)
                .putExtra("package", getPackageName()));
    }

    private String findEnginePackage() {
        for (String pkg : ENGINE_PACKAGES) {
            try {
                getPackageManager().getPackageInfo(pkg, 0);
                return pkg;
            } catch (PackageManager.NameNotFoundException ignored) {}
        }
        return null;
    }

    private void setRunButtonEnabled(boolean enabled) {
        runButton.setEnabled(enabled);
        runButton.setAlpha(enabled ? 1f : 0.5f);
    }

    private void setStatus(int resId, int colorResId) {
        statusText.setText(resId);
        statusText.setTextColor(getColor(colorResId));
        statusText.setVisibility(View.VISIBLE);
    }

    private void saveArgs(String args) {
        prefs.edit().putString(KEY_ARGV, args).apply();
    }

    private void openUrl(String url) {
        startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(url)));
    }

    private void hideKeyboard(View view) {
        InputMethodManager imm = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
        if (imm != null) imm.hideSoftInputFromWindow(view.getWindowToken(), 0);
    }
}