//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.text.SpannableString;
import android.text.method.LinkMovementMethod;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;

import com.google.android.material.floatingactionbutton.FloatingActionButton;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.Set;

public class PasswordListActivity extends AppCompatActivity
{
    private static String TAG = PasswordListActivity.class.getSimpleName();

    private ArrayList<String> _passwords;
    private PasswordListAdapter _adapter;
    private Intent _resultIntent = new Intent();

    private class PasswordListAdapter extends ArrayAdapter<String>
    {
        private Context _ctx;
        private int _resId;

        public PasswordListAdapter(Context ctx, int resId, ArrayList<String> list)
        {
            super(ctx, resId, list);
            _ctx = ctx;
            _resId = resId;
        }

        private String obfuscatePassword(String password)
        {
            // Check for null or too short input
            if (password == null || password.length() <= 4)
            {
                return password;
            }

            int length = password.length();
            StringBuilder obfuscated = new StringBuilder();

            // Append the first two characters
            obfuscated.append(password.substring(0, 2));

            // Replace the middle characters with asterisks
            for (int i = 2; i < length - 2; i++)
            {
                obfuscated.append('*');
            }

            // Append the last two characters
            obfuscated.append(password.substring(length - 2));

            return obfuscated.toString();
        }

        @NonNull
        @Override
        public View getView(int position, @Nullable View convertView, @NonNull ViewGroup parent)
        {
            LayoutInflater inflator = LayoutInflater.from(_ctx);
            convertView = inflator.inflate(_resId, parent, false);

            final String item = getItem(position);
            final String displayedItem;

            displayedItem = obfuscatePassword(item);

            ((TextView)convertView.findViewById(R.id.tvPassword)).setText(displayedItem);
            //((TextView)convertView.findViewById(R.id.tvDescription)).setText("---no description---");

            convertView.setOnClickListener(new View.OnClickListener()
            {
                @Override
                public void onClick(View v)
                {
                    viewPassword(item);
                }
            });

            convertView.findViewById(R.id.ivDeletePassword).setOnClickListener(new View.OnClickListener()
            {
                @Override
                public void onClick(View v)
                {
                    confirmDeletePassword(item);
                }
            });

            /*
            if(item.getFileName().contains(Constants.INTERNAL_DEFAULT_CERTSTORE_FN) || item.getFileName().contains(Constants.RTS_FACTORY_CERTSTORE_FN))
            {
                convertView.findViewById(R.id.ivDeleteCertStore).setVisibility(View.INVISIBLE);
            }
            */

            return convertView;
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_password_list);

        FloatingActionButton fabAdd = findViewById(R.id.fabAdd);
        fabAdd.setOnClickListener(new View.OnClickListener()
        {
            @Override
            public void onClick(View view)
            {
                showPasswordDialog();
            }
        });

        loadPasswords();

        _adapter = new PasswordListAdapter(this, R.layout.password_list_entry, _passwords);
        ListView lv = findViewById(R.id.lvPasswords);
        lv.setAdapter(_adapter);

        setupActionBar();
    }

    private void setupActionBar()
    {
        ActionBar actionBar = getSupportActionBar();
        if (actionBar != null)
        {
            actionBar.setDisplayHomeAsUpEnabled(true);
        }
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item)
    {
        int id = item.getItemId();
        if (id == android.R.id.home)
        {
            onBackPressed();
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent intent)
    {
        Globals.getLogger().d(TAG, "onActivityResult");//NON-NLS

        if(resultCode == RESULT_OK)
        {
            /*
            if(requestCode == Constants.PICK_CERTSTORE_FILE_REQUEST_CODE)
            {
                importCertStoreFromUri(intent.getData());
            }
             */
        }
    }

    private void loadPasswords()
    {
        Set<String> defaultSet = new HashSet<>();
        HashSet<String> theSet = (HashSet) Globals.getSharedPreferences().getStringSet(PreferenceKeys.USER_CERT_STORE_PASSWORD_SET, defaultSet);

        _passwords = new ArrayList<>();
        for(String pwd : theSet)
        {
            _passwords.add(pwd);
        }
    }

    private void savePasswords()
    {
        Set<String> theSet = new HashSet<>();

        for(String pwd : _passwords)
        {
            theSet.add(pwd);
        }

        Globals.getSharedPreferencesEditor().putStringSet(PreferenceKeys.USER_CERT_STORE_PASSWORD_SET, theSet);
        Globals.getSharedPreferencesEditor().apply();
    }

    private void viewPassword(final String pwd)
    {
        Toast.makeText(this, getString(R.string.passwords_cannot_be_viewed_or_edited_only_added_or_removed), Toast.LENGTH_LONG).show();
    }

    private void confirmDeletePassword(final String pwd)
    {
        String s = getString(R.string.are_you_sure_you_want_to_delete_this_password);

        final TextView message = new TextView(this);
        final SpannableString ss = new SpannableString(s);

        message.setText(ss);
        message.setMovementMethod(LinkMovementMethod.getInstance());
        message.setPadding(32, 32, 32, 32);

        AlertDialog dlg = new AlertDialog.Builder(this)
                .setTitle(R.string.delete_password)
                .setCancelable(false)
                .setPositiveButton(R.string.button_yes, new DialogInterface.OnClickListener()
                {
                    @Override
                    public void onClick(DialogInterface dialogInterface, int i)
                    {
                        for(String p : _passwords)
                        {
                            if(pwd.compareTo(p) == 0)
                            {
                                try
                                {
                                    _passwords.remove(p);
                                    _adapter.notifyDataSetChanged();
                                    savePasswords();
                                }
                                catch (Exception e)
                                {
                                    e.printStackTrace();
                                }

                                break;
                            }
                        }
                    }
                }).setNegativeButton(R.string.button_cancel, new DialogInterface.OnClickListener()
                {
                    @Override
                    public void onClick(DialogInterface dialogInterface, int i)
                    {
                    }
                }).setView(message).create();

        dlg.show();
    }

    private void showPasswordDialog()
    {
        LayoutInflater layoutInflater = LayoutInflater.from(PasswordListActivity.this);
        View promptView = layoutInflater.inflate(R.layout.password_entry_dialog, null);
        AlertDialog.Builder alertDialogBuilder = new AlertDialog.Builder(PasswordListActivity.this);
        alertDialogBuilder.setView(promptView);

        final EditText editText = promptView.findViewById(R.id.etPassword);

        alertDialogBuilder.setCancelable(false)
                .setPositiveButton(R.string.button_ok, new DialogInterface.OnClickListener()
                {
                    public void onClick(DialogInterface dialog, int id)
                    {
                        String pwd = editText.getText().toString();
                        _passwords.add(pwd);
                        _adapter.notifyDataSetChanged();
                        savePasswords();
                    }
                })
                .setNegativeButton(R.string.cancel,
                        new DialogInterface.OnClickListener()
                        {
                            public void onClick(DialogInterface dialog, int id)
                            {
                                dialog.cancel();
                            }
                        });

        AlertDialog alert = alertDialogBuilder.create();
        alert.show();
    }
}
