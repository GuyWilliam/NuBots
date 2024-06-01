import React, { ReactNode } from "react";
import classNames from "classnames";
import { action } from "mobx";
import { observer } from "mobx-react";

import style from "./style.module.css";
import { Option } from "./view";

export type SelectOptionProps = {
  className?: string;
  option: Option;
  showIconPadding: boolean;
  icon?: ReactNode;
  isSelected: boolean;
  onSelect(option: Option): void;
};

@observer
export class SelectOption extends React.Component<SelectOptionProps> {
  render(): JSX.Element {
    const { className, option, showIconPadding, icon, isSelected } = this.props;

    return (
      <div
        className={classNames([
          className,
          "items-center p-2 ",
          isSelected
            ? "bg-blue-600 hover:bg-blue-700 dark:hover:bg-blue-500 text-white"
            : "hover:bg-gray-200 dark:hover:bg-gray-600",
        ])}
        onClick={this.onSelect}
      >
        {showIconPadding || icon ? (
          <span className={"w-[20px] h-[20px] mr-[8px]"}>
            <div className={"w-full h-full inline"}>{icon}</div>
          </span>
        ) : null}
        {option.label}
      </div>
    );
  }

  @action.bound
  private onSelect() {
    this.props.onSelect(this.props.option);
  }
}
