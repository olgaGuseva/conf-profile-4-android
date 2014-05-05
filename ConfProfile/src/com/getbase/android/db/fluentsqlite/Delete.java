package com.getbase.android.db.fluentsqlite;

import static com.google.common.base.Preconditions.checkNotNull;

import com.getbase.android.db.fluentsqlite.Expressions.Expression;
import com.google.common.base.Functions;
import com.google.common.base.Joiner;
import com.google.common.collect.FluentIterable;
import com.google.common.collect.Lists;

import android.database.sqlite.SQLiteDatabase;

import java.util.Arrays;
import java.util.List;

public class Delete implements DeleteTableSelector {
  private String mTable;
  private List<String> mSelections = Lists.newArrayList();
  private List<Object> mSelectionArgs = Lists.newArrayList();

  private Delete() {
  }

  public static DeleteTableSelector delete() {
    return new Delete();
  }

  public int perform(SQLiteDatabase db) {
    return db.delete(
        mTable,
        Joiner.on(" AND ").join(mSelections),
        FluentIterable.from(mSelectionArgs).transform(Functions.toStringFunction()).toArray(String.class)
    );
  }

  @Override
  public Delete from(String table) {
    mTable = checkNotNull(table);
    return this;
  }

  public Delete where(String selection, Object... selectionArgs) {
    mSelections.add("(" + selection + ")");
    mSelectionArgs.addAll(Arrays.asList(selectionArgs));

    return this;
  }

  public Delete where(Expression expression, Object... selectionArgs) {
    return where(expression.toRawSql(), expression.getMergedArgs(selectionArgs));
  }
}
