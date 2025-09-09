package com.rallytac.engageandroid;

import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.EditText;

import androidx.appcompat.app.AppCompatActivity;

import org.json.JSONObject;

public class JsonEditorActivity extends AppCompatActivity
{
    public static final String JSON_DATA = "JSON_DATA";

    private String _json = null;
    private EditText _etEditor = null;

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_json_editor);

        Intent intent = getIntent();
        if(intent != null)
        {
            _json = intent.getStringExtra(JSON_DATA);
        }

        // Try to format it
        if(!Utils.isEmptyString(_json))
        {
            try
            {
                JSONObject jsonObject = new JSONObject(_json);
                String formatted = jsonObject.toString(3);
                _json = formatted;
            }
            catch (Exception e)
            {
            }
        }

        _etEditor = findViewById(R.id.etEditor);
        _etEditor.setText(_json);
    }

    private boolean isJsonValid(String s)
    {
        boolean rc = false;

        try
        {
            String json = _etEditor.getText().toString();
            JSONObject jsonObject = new JSONObject(json);
            rc = true;
        }
        catch (Exception e)
        {
            rc = false;
        }

        return rc;
    }

    public void onClickClear(View view)
    {
        _etEditor.setText(null);
    }

    public void onClickCancel(View view)
    {
        finish();
    }

    public void onClickValidate(View view)
    {
        if(isJsonValid(_etEditor.getText().toString()))
        {
            Utils.showShortPopupMsg(this, getString(R.string.json_validation_passed));
            viewJsonPretty();
        }
        else
        {
            Utils.showShortPopupMsg(this, getString(R.string.json_validation_failed));
        }
    }

    public void onClickSave(View view)
    {
        String json = _etEditor.getText().toString();
        if(Utils.isEmptyString(json) || isJsonValid(json))
        {
            Intent resultIntent = new Intent();
            resultIntent.putExtra(JSON_DATA, json);
            setResult(RESULT_OK, resultIntent);
            finish();
        }
        else
        {
            Utils.showShortPopupMsg(this, getString(R.string.save_operation_aborted_as_json_validation_failed));
        }
    }

    private void viewJsonPretty()
    {
        // TODO: viewJsonPretty
        /*
        String json = _etEditor.getText().toString();
        if(Utils.isEmptyString(json))
        {
            Utils.showShortPopupMsg(this, getString(R.string.no_json_to_view));
            return;
        }

        if(!isJsonValid(json))
        {
            Utils.showShortPopupMsg(this, getString(R.string.json_is_invalid));
            return;
        }

        LayoutInflater layoutInflater = LayoutInflater.from(this);

        View promptView = layoutInflater.inflate(R.layout.json_view_dialog, null);
        JsonViewLayout jsonViewLayout = promptView.findViewById(R.id.jsonView);
        jsonViewLayout.bindJson(json);

        AlertDialog.Builder alertDialogBuilder = new AlertDialog.Builder(this);
        alertDialogBuilder.setView(promptView);


        alertDialogBuilder.setCancelable(false);
        alertDialogBuilder.setPositiveButton(R.string.button_close,null);
        AlertDialog alert = alertDialogBuilder.create();
        alert.show();
        */
    }
}
