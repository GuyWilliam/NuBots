import React from "react";
import { ChangeEvent } from "react";
import classNames from "classnames";

import { Icon } from "../icon/view";

export interface CheckboxProps {
  checked: boolean;
  disabled?: boolean;
  onChange(event: ChangeEvent<HTMLInputElement>): void;
}

export const Checkbox = (props: CheckboxProps) => {
  const { checked, disabled, onChange } = props;

  return (
    <span className={classNames("checkbox inline-block w-5 h-5 relative", disabled ? "opacity-60" : "cursor-pointer")}>
      <input
        type="checkbox"
        className="appearance-none h-full w-full m-0 p-0 outline-none absolute"
        checked={checked}
        disabled={disabled}
        onChange={onChange}
      />
      <span
        className={classNames(
          "border-2 rounded-sm w-full h-full p-2 absolute transition-colors",
          checked ? "bg-nusight-500 border-nusight-500" : "bg-white border-[#757575]",
        )}
      >
        <Icon
          className={classNames("text-white transition-opacity text-[1.125rem]", checked ? "opacity-100" : "opacity-0")}
          weight={700}
        >
          check
        </Icon>
      </span>
    </span>
  );
};
